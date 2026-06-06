I’m working on the Webserv project (42 school), and my current task is implementing the Routing system.

Act as a senior C++ engineer who has already completed multiple Webserv projects and understands how nginx-like routing works internally.

I want a complete implementation guide, not just a theory lesson.

---

# 1. Routing Fundamentals

What routing is

- Routing is the component that maps an incoming HTTP request (method + URL + headers) to the action the server must take: serve a static file, run a CGI, return a redirect, return an error, or generate a directory listing.

Why routing exists

- To interpret location/server configuration and apply rules that transform a request URL into a canonical server-side action and filesystem path. It expresses server behavior (which resources are exposed, which methods are allowed, which pages are index pages, redirects, aliases, CGI hooks, etc.).

At which stage of the request lifecycle routing happens

- Routing runs after the Request Parser finishes producing a parsed request object and before Response Builder or handler code chooses how to generate the body. Sequence:

Client
→ Socket
→ Request Parser
→ Router
→ Response Builder
→ Client

What the routing module receives as input

- A parsed request object: method, raw URI, path, query string, headers (Host, etc.), and optionally the body.
- Server configuration context that identifies which server block applies (based on Host header and port), and the server's list of location blocks.
- Filesystem access helpers (stat/exists/read permissions) or an abstraction to query filesystem state.

What the routing module should return as output

- A RouteResult (see Section 7) describing the route action: resolved filesystem path, matched location config, selected handler type (static, index, autoindex, CGI, redirect, error), status code if immediate (e.g., 301), allowed methods list, CGI parameters if applicable, and any flags (autoindex on, requires authentication, etc.).

Where routing fits in (visual)

Client → Socket → Request Parser → Router → Response Builder → Client

---

# 2. Where Should I Implement Routing?

Which class should own the routing logic

- Create a dedicated `Router` class. Keep routing logic out of `Request`, `Response`, and `Server` to preserve single responsibility and testability.

Should I create a Router class

- Yes. `Router` coordinates server selection, location matching, path resolution, and returns a `RouteResult`. It's the single place to reason about mapping requests → actions.

Responsibilities by class

- `Request`:
  - Should know: parsed method, raw URI, path portion, query string, HTTP version, headers map, body presence/length.
  - Should NOT know: server config, location matching, filesystem paths, or response formation.

- `Router`:
  - Should know: server configs (server blocks), location configs for the selected server, and filesystem helpers.
  - Responsibilities: select server, match location, apply location rules (methods, redirects, alias/root handling), resolve URL to filesystem path, validate path existence/type (file/dir), decide final action (static file, index, autoindex, CGI, redirect, error), produce `RouteResult`.
  - Should NOT know: how to serialize HTTP responses or write bytes to sockets.

- `Server`:
  - Should know: its `server_config` struct: host(s), port, root, default index list, error pages, location list.
  - Responsibilities: provide its config to `Router`, optionally register with multiplexer/listener; does not decide routing itself.
  - Should NOT know: per-request matching logic beyond configuration storage.

- `Response` (or ResponseBuilder)
  - Should know: how to produce headers/body based on `RouteResult` (read file, call CGI, build directory HTML, set status codes and headers like `Location`, `Allow`, `Content-Type`).
  - Should NOT know: how to pick the correct location or compute the filesystem path — it should accept `RouteResult` and produce the HTTP payload.

Separation summary

- Router returns an explicit `RouteResult` DTO; ResponseBuilder consumes it to produce the HTTP reply. This keeps responsibilities clean and testable.

---

# 3. Data Required By The Router

Request data

- Method (GET, POST, DELETE, etc.)
  - Why: method validation and deciding if body/CGI is required
  - How: compare with location `allowed_methods`, determine 405 if not allowed

- URI (raw and percent-decoded path)
  - Why: main routing input — used for location matching and path resolution
  - How: parse and split into path + query string; canonicalize/percent-decode for filesystem mapping

- Path (normalized, no query)
  - Why: used for location matching and building filesystem path
  - How: normalize (collapse //, resolve ./, but do NOT resolve .. until you validate against root; percent-decode first)

- Query string
  - Why: may be passed to CGI, and may affect resource handling in some dynamic routes
  - How: attach to CGI env or leave in `RouteResult` for ResponseBuilder

- Headers (Host, Authorization, etc.)
  - Why: `Host` identifies the server block; `Authorization` may affect access decisions; others (Accept, Range) affect response later
  - How: use `Host` header to pick server; pass other headers to CGI as required

Server data

- Host(s) and Port
  - Why: server selection when multiple virtual hosts exist
  - How: match Host header (and port) to server blocks. If none match, use default server

- Root
  - Why: base filesystem path for locations that inherit server root
  - How: used during path resolution when a location uses `root` or inherits server `root`

- Index list (index files)
  - Why: when a directory is requested, look for index files (index.html, index.php)
  - How: iterate index list to find the first existing file

- Error pages
  - Why: custom error file paths for specific status codes
  - How: if routing yields a status that has a configured error page, include that path in `RouteResult`

Location data (per location block)

- Path (location prefix or exact)
  - Why: how to match request URIs to this location
  - How: matching algorithm (see Section 5)

- Root (optional)
  - Why: override server root for that location
  - How: used when mapping URL to filesystem path

- Alias (optional)
  - Why: perform alternate mapping of URL to a different filesystem subtree
  - How: replace the location prefix in the requested path with alias value, without appending the location path

- Index
  - Why: local per-location index file preference
  - How: used when directory resolves to index lookup

- Allowed methods
  - Why: block methods not supported (405)
  - How: Router checks method against this list early

- Redirect (return 301/302)
  - Why: location configured as redirect
  - How: produce `RouteResult` with redirect info and appropriate status

- CGI (script handler)
  - Why: this location should be handled by a CGI script or FastCGI backend
  - How: Router marks route as CGI and includes script path and env details

- Autoindex flag
  - Why: whether to generate a directory listing when no index is found
  - How: Router sets a flag in `RouteResult` to indicate autoindex behavior

Filesystem data

- Physical file path
  - Why: actual file to open and stream or execute
  - How: Router computes final path and includes it in `RouteResult`

- File existence / type (file vs directory)
  - Why: to decide index lookup, autoindex, or 404
  - How: Router queries the filesystem (stat) and branches accordingly

- Permissions (read/execute) — optional but recommended
  - Why: if lacking read permission for static file or execute for CGI, return 403
  - How: check filesystem permissions before finalizing route

---

# 4. Routing Algorithm (Core Part)

High level: input a parsed HTTP request, return a `RouteResult` (DTO) describing the server action.

RouteResult will be described in Section 7. Here is the algorithm broken into concrete steps.

Step 1: Receive request

- Input: `Request` object (method, normalized path, query, headers)
- Processing: parse Host header if needed; ensure path is percent-decoded and normalized
- Output: canonical request path and method available for routing

Step 2: Select matching server

- Input: Host header, incoming port/socket
- Processing: loop server definitions:
  - If server has `server_name` matching Host header, choose it
  - If multiple match, use exact host match then wildcard rules as in config precedence
  - If none match, use default server for the listening socket
- Output: `ServerConfig` object

Step 3: Select matching location

- Input: canonical request path and server's location list
- Processing: use the location matching algorithm (Section 5). Typically:
  - Test exact matches first (if your config supports `=` exact blocks)
  - Test prefix matches and track the longest matching prefix
  - Apply special modifiers (e.g., `^~`, `~` regex) if you implement them
- Output: `LocationConfig` and the matched prefix length

Step 4: Apply location rules

- Input: selected `LocationConfig`, `Request`
- Processing:
  - If location declares a `redirect`, produce `RouteResult` with redirect info and exit.
  - Validate method: if method not allowed → return immediate 405 with `Allow` header listing allowed methods.
  - Determine effective root/alias: location `alias` (higher priority) or `root` (location then server default).
  - Decide whether this location is CGI: mark accordingly.
- Output: intermediate route decision state

Step 5: Build filesystem path

- Input: request path, matched location prefix, effective root/alias
- Processing: two main behaviors:
  - root-style mapping: filesystem_path = join(effective_root, path_after_prefix)
  - alias-style mapping: filesystem_path = join(alias_value, path_after_prefix) — careful: alias replaces the location prefix, not just appends
  - Normalize and protect against path traversal (see Sec. 9)
- Output: candidate filesystem path

Step 6: Validate path

- Input: candidate filesystem path
- Processing:
  - stat the path: does it exist? is it a file or directory?
  - If it is a directory:
    - Look for index files (location index list or server index list). If index found, set path to that file and type=file.
    - If no index and autoindex enabled → mark autoindex action.
    - If no index and autoindex disabled → return 403 or 404 depending on config (commonly 403 or 404; choose 403 if resource exists but listing forbidden).
  - If file exists: check read permissions. If unreadable → 403.
  - If not exist → 404.
- Output: validated path type and potential fallback decisions

Step 7: Determine action

- Input: validated path and location flags
- Processing: final decision tree:
  - If redirect was set earlier → return redirect RouteResult
  - If route is CGI → return CGI RouteResult with script path and env
  - If path resolves to a file → return static file RouteResult (status 200)
  - If path resolves to a directory + autoindex enabled → return autoindex RouteResult
  - If missing and no fallback → return 404 (or custom error page if configured)
- Output: completed `RouteResult`

Step 8: Return RouteResult

- Input: final action data
- Processing: populate headers that routing determines (e.g., `Allow` for 405, `Location` for 301/302), embed pointers to custom error pages if present.
- Output: `RouteResult` handed off to ResponseBuilder

Each step should be unit-testable. Keep filesystem access abstracted behind small helper methods to allow mocking.

---

# 5. Location Matching Algorithm

Goal: pick the best matching location block for a request path.

Rules (nginx-inspired, simplified for Webserv):

- Exact match (if implemented) has highest priority (e.g., `location = /foo`)
- Prefix match: test all prefix locations; the longest matching prefix wins.
- Regex (`~`, `~*`) and other modifiers can be supported later; for initial implementation, only prefix and exact are enough.

Example locations:

- `location /` (prefix `/`)
- `location /images` (prefix `/images`)
- `location /images/icons` (prefix `/images/icons`)

Request: `GET /images/icons/logo.png`

Every location checked (prefix matching):

- `/` → matches (prefix `/` always matches)
- `/images` → matches because `/images` is the start of `/images/icons/logo.png`
- `/images/icons` → matches because `/images/icons` is the start

Which one wins

- Longest prefix wins. `/images/icons` is length 14, `/images` is shorter, `/` is shortest. So `/images/icons` wins.

Exact match behavior

- If a location is defined with exact semantics (e.g., `=`), only return it when the requested path exactly matches, otherwise ignore it.

Prefix matching notes

- Match boundaries: a prefix match should match path segments, but you can treat it as simple string prefix. If you want stricter rules, ensure `/foo` does not erroneously match `/foobar` unless intended.

Diagrams (simple):

Request path: /images/icons/logo.png

Locations tested: / → /images → /images/icons

Winner: /images/icons

Mermaid diagram (conceptual):

```mermaid
flowchart LR
    A(Request:/images/icons/logo.png) --> B[/images/icons]
    A --> C[/images]
    A --> D[/]
    B --> E[Selected]
```

---

# 6. Path Resolution

How URL paths become filesystem paths

Rules & concepts

- `root` semantics: append the part of the request path after the location prefix to the `root`. Example:
  - `location /images { root /var/www/assets; }`
  - Request: `/images/logo.png` → path_after_prefix=`/logo.png` → filesystem `/var/www/assets/images/logo.png` if you append the request path (note: nginx behavior with `root` appends the full request path — see caveat below)

- `alias` semantics: replace the location prefix with the alias. Example:
  - `location /images { alias /var/www/assets; }`
  - Request: `/images/logo.png` → filesystem `/var/www/assets/logo.png` (location prefix removed)

- index: when filesystem path resolves to a directory, router should look for configured index files in order (e.g., `index.html`, `index.php`). If an index is found, return that file path.

Caveats & common pitfalls

- With `root`, if you define `location /images` and `root /var/www/assets`, some server implementations append the request path to the `root` without removing the prefix, giving `/var/www/assets/images/logo.png`. That's a common source of confusion; `alias` is used when you want to map `/images` directly to `/var/www/assets` (so file path has no `/images` subfolder).

Examples

1) Using `root`

Config: `location / { root /var/www; }`

Request: `GET /images/logo.png` → path_after_prefix=`/images/logo.png` → filesystem `/var/www/images/logo.png`

2) Using `alias`

Config: `location /images { alias /var/www/assets; }`

Request: `GET /images/logo.png` → filesystem `/var/www/assets/logo.png`

3) Directory with index

Config: `location / { root /var/www; index index.html; }`

Request: `GET /` → filesystem `/var/www/index.html` (if exists) → serve file

---

# 7. RouteResult Design

Design a small struct/class to represent the output of routing. Example C++ struct:

```cpp
struct RouteResult {
    int status; // default 0 means continue; 200/301/404/403/405 etc.
    std::string filesystem_path; // resolved absolute path when applicable
    const LocationConfig* matched_location; // pointer/ref to matched config
    const ServerConfig* matched_server;
    bool is_cgi = false;
    bool is_autoindex = false;
    bool is_directory = false;
    bool is_file = false;
    bool is_redirect = false;
    std::string redirect_location; // for 301/302
    std::vector<std::string> allow_methods; // for 405
    std::string cgi_script_path; // if CGI
    std::map<std::string,std::string> cgi_env; // env for CGI
    std::string error_page_path; // if custom error page should be used
    std::string reason; // optional human-friendly reason used in logs
};
```

Why each field exists

- `status`: indicates immediate responses (redirects, method not allowed, not found) or 0/200 for normal flow.
- `filesystem_path`: the path the ResponseBuilder should open or execute.
- `matched_location` and `matched_server`: allow ResponseBuilder to inspect headers/config (e.g., custom error pages, autoindex templates)
- `is_cgi`: tells ResponseBuilder to run a CGI process instead of reading a static file.
- `is_autoindex`: instructs ResponseBuilder to generate a directory listing.
- `is_redirect` and `redirect_location`: instructs ResponseBuilder to return a `Location` header and appropriate status.
- `allow_methods`: for 405 responses.
- `cgi_*` fields: required to setup CGI environment and script path.
- `error_page_path`: if routing resolved an error and a custom error page was configured.

---

# 8. Edge Cases (Very Important)

For each edge case, explain behavior and HTTP status.

- File does not exist
  - Behavior: routing returns 404 Not Found. If server has configured custom 404 page, include its path in `RouteResult`.
  - Status: 404

- Directory requested
  - Behavior: stat shows directory. If index file exists → serve index. If not and autoindex enabled → RouteResult with autoindex. If not autoindex → 403 (or 404 depending on desired semantics). I recommend 403 (Forbidden) if directory exists but listing not allowed.
  - Status: 200 (if index), 403 (no index & autoindex off), 200 (autoindex response)

- Missing index file
  - Behavior: if autoindex on → listing; else 403/404. Choose 403 to indicate access denied to directory contents.
  - Status: 403 or 404 (commonly 403)

- Autoindex enabled
  - Behavior: produce `RouteResult` with `is_autoindex=true`. ResponseBuilder generates HTML listing.
  - Status: 200

- Autoindex disabled
  - Behavior: return 403 (or 404 per your policy). Recommend 403.
  - Status: 403

- Method not allowed
  - Behavior: Router checks `allowed_methods` and returns 405 with `Allow` header listing allowed methods.
  - Status: 405

- Invalid location (no matching location) — should fall back to `location /` or server default
  - Behavior: use `location /` or default behavior (likely 404 if root mapping yields nothing)
  - Status: 404 if resource missing

- Redirect location
  - Behavior: Router produces a `RouteResult` with `is_redirect=true` and `redirect_location`. ResponseBuilder returns 301/302 with `Location` header.
  - Status: configured redirect code (301/302)

- CGI route
  - Behavior: Router marks route as CGI and sets script path and env. ResponseBuilder spawns CGI process or forwards to FastCGI.
  - Status: 200 (or whatever CGI outputs); routing only marks the intention.

- Root missing
  - Behavior: if root configured but folder missing, treat requests under that root as 404 (or 500 if you consider config error). Prefer 404 for requested resources; log a config warning about missing root.
  - Status: 404

- Alias misconfiguration
  - Behavior: incorrectly used alias may map to a file outside intended dir. Router must ensure alias mapping does not allow escape outside its intended tree.
  - Status: 403 (if traversal detected) or 500 if config is internally inconsistent — prefer 403 and log.

- Path traversal attempts (../)
  - Behavior: Normalize and reject attempts that resolve outside the configured `root`/`alias`. Return 403 Forbidden. Do not allow traversal to expose files outside root.
  - Status: 403

- Multiple matching locations
  - Behavior: longest prefix wins (see Section 5). If exact match present, prefer it per config rules.
  - Status: follow chosen route's result

- Empty URI
  - Behavior: treat as `/`. Run index lookup on root.
  - Status: 200 (index) or 404/403 as applicable

- URI ending with `/`
  - Behavior: resolve to directory. Look up index files. If you prefer directory redirects, you may choose to redirect `GET /dir` -> `GET /dir/` but only necessary when relative resources depend on trailing slash.
  - Status: 301 redirect (if you canonicalize with slash) or index/autoindex handling.

For every edge case, ensure Router logs the decision and includes `reason` in the `RouteResult` for easier debugging.

---

# 9. Security Considerations

- Path traversal protection
  - Always percent-decode the path and normalize (remove ./ and collapse //) before joining with root/alias.
  - After computing the candidate filesystem path, resolve its canonical path (realpath) and verify it starts with the expected `root` or `alias` base path. If not, reject with 403.

- Canonical path validation
  - Use `realpath()` or `std::filesystem::canonical` where available, but treat symlinks carefully — if your server must respect symlinks inside root, ensure canonical check still stays within allowable tree.

- Preventing access outside root
  - Always check computed canonical path has prefix equal to canonical root/alias. If not, block access.

- Dangerous routing mistakes
  - Using `alias` incorrectly: ensure you understand alias semantics vs root — tests should cover both.
  - Overly permissive `autoindex`: exposing internal files. Prefer default off.
  - Allowing `DELETE` globally: restrict dangerous methods explicitly.

---

# 10. Real Example Walkthrough

Config:

```
server {
root /var/www;

```
location / {
    index index.html;
}

location /images {
    root /var/www/assets;
}

}
```

Request: GET /images/logo.png

Walk through every routing step until the final RouteResult.

1) Request parser yields method=GET, path=/images/logo.png, Host header -> server selection choose this server
2) Router inspects server's locations: `/` and `/images`
3) Location matching: `/images` and `/` both match; `/images` has longer prefix, so it wins
4) Apply location rules: location `/images` declares `root /var/www/assets`.
   - Decide mapping semantics: using `root` means filesystem = join(root, path_after_prefix). `path_after_prefix` is `/logo.png`? (Depending on `root` semantics implementation — see caveat). For our guide we'll assume `root` appends the full request path after the location prefix is used as in many simple servers, giving filesystem `/var/www/assets/images/logo.png`.
5) Build candidate filesystem path: `/var/www/assets/images/logo.png`
6) Validate path: stat the path
   - If file exists and readable → RouteResult.is_file=true, filesystem_path set to that file, status 200
   - If file not found → 404 (but if you intended `/var/www/assets/logo.png` with alias behavior, check config and consider using `alias`)
7) Return RouteResult with matched_location `/images`, matched_server, filesystem_path, is_file=true

---

# 11. Implementation Roadmap

Phase 1: Basic location matching

- Implement `Router` class, `RouteResult` struct
- Implement server selection by Host header
- Implement prefix-based location matching (longest prefix wins)

Phase 2: Path resolution

- Implement `root` and `alias` mappings (pick default behavior and document it)
- Implement path normalization and percent-decoding
- Add canonical path / realpath checks for security

Phase 3: Method validation

- Implement `allowed_methods` checks and 405 responses with `Allow` header

Phase 4: Index handling

- Implement directory detection (stat)
- Implement index file search using location then server index list
- Integrate index results into `RouteResult`

Phase 5: Autoindex

- Implement `is_autoindex` flag in `RouteResult`
- Implement a simple directory listing generator in ResponseBuilder

Phase 6: CGI support

- Mark locations as CGI-capable
- Provide `RouteResult` fields needed to spawn CGI (script path, cgi_env)
- Implement CGI execution in ResponseBuilder (spawn child, pipe env/body, read output)

Phase 7: Error handling and custom error pages

- Wire custom error pages into routing (fill `error_page_path` in `RouteResult`)
- Ensure ResponseBuilder prefers custom error pages when present

Phase 8: Robust tests & edge cases

- Add unit tests for:
  - Location matching (prefix cases, exact cases)
  - Path resolution for `root` vs `alias`
  - Path traversal attempts
  - Index and autoindex flows
  - Method not allowed
  - CGI route behavior (mocked)

Why this order

- Each phase builds a required foundation. Location matching and path resolution are prerequisites for index, autoindex, and CGI. Method validation is cheap and early, preventing unnecessary filesystem work for disallowed methods. CGI and error handling are more complex and rely on a solid mapping foundation.

---

# Appendix: Implementation hints & examples (C++)

- Keep a small filesystem helper API so routing can call `stat`, `is_dir`, `is_file`, `is_readable`, and `canonicalize` without embedding system calls throughout the Router.
- Example Router method signatures:

```cpp
class Router {
public:
    Router(const std::vector<ServerConfig>& servers, Filesystem& fs);
    RouteResult route(const Request& req, int incoming_port);
private:
    const ServerConfig* select_server(const Request& req, int incoming_port);
    const LocationConfig* match_location(const ServerConfig& s, const std::string& path);
    RouteResult apply_location(const Request& req, const ServerConfig& s, const LocationConfig& l);
    // helpers
};
```

- Testing tip: create fake `Filesystem` implementations for unit tests that simulate files, directories, permissions, and symlinks.
- Logging tip: include `reason` in `RouteResult` for every non-200 decision to speed debugging.

---

How to THINK while building routing (short)

- Always map inputs → config → filesystem → action. For each decision, ask "what do I need to know to choose between A and B?" If it involves filesystem state, abstract it and write tests that mock it.
- Implement the simplest useful behavior first (prefix matching, root append semantics), then add complexity (alias, regex, modifiers) once tests and infra exist.

If you want, I can now:

- Implement `Router` and `RouteResult` skeleton in C++ in `src/Server` and add unit tests and filesystem helper.
- Or produce a small test harness demonstrating routing decisions.

Tell me which next step you prefer and I will implement it.
