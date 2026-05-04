# Webserv Big Picture Roadmap: From Config to Response

Think of building `webserv` like building a **restaurant from scratch**. You don't just start cooking — you first design the menu, build the kitchen, set up tables, hire staff, and *then* serve customers. Your web server follows the exact same logic.

---

## 1. Global Flow (Big Picture)

Here is the **entire lifecycle** of your server in one diagram:

```
┌─────────────────────────────────────────────────────────────────────┐
│                         STARTUP PHASE                                │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────────────┐  │
│  │ Read config │ → │ Parse into  │ → │ Create ServerConfig      │  │
│  │   file      │    │   tokens    │    │  & LocationConfig       │  │
│  └─────────────┘    └─────────────┘    └─────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                                  ↓
┌─────────────────────────────────────────────────────────────────────┐
│                    SERVER INITIALIZATION                               │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────────────┐  │
│  │ For each    │ → │ socket()    │ → │ bind() + listen()        │  │
│  │  server{}   │    │  create fd  │    │  on IP:port             │  │
│  └─────────────┘    └─────────────┘    └─────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                                  ↓
┌──────────────────────────────────────────────────────────────────────┐
│                      MAIN LOOP (Runtime)                             │
│  ┌─────────────────────────────────────────────────────────────────┐ │
│  │  select()/poll()/epoll() waits for activity on ALL sockets      │ │
│  └─────────────────────────────────────────────────────────────────┘ │
│                              ↓                                       │
│         ┌────────────────────┴────────────────────┐                  │
│         ↓                                         ↓                  │
│  ┌─────────────┐                         ┌─────────────────┐         │
│  │ New client  │                         │ Existing client │         │
│  │  connect?   │                         │   sent data?    │         │
│  └─────────────┘                         └─────────────────┘         │
│         ↓                                         ↓                  │
│    accept() → add to                        recv() → parse           │
│    monitoring set                           → build response         │
│                                                    → send()          │
│                                                    → close()         │
└──────────────────────────────────────────────────────────────────────┘
```

**The golden rule:** Your server is just a loop that waits → detects → handles → repeats.

---

## 2. Startup Phase: Reading the Blueprint

### What Happens When the Server Starts?

You run `./webserv config.conf`. The server wakes up and reads its **blueprint**.

### The Config File (Your Blueprint)

```nginx
server {
    listen 8080;
    server_name localhost;
    root /var/www/html;
    index index.html;
    client_max_body_size 1M;

    location / {
        allow_methods GET POST;
    }

    location /uploads {
        allow_methods POST;
        upload_path /var/www/uploads;
    }

    location /cgi-bin {
        allow_methods GET POST;
        cgi_extension .py;
        cgi_path /usr/bin/python3;
    }

    error_page 404 /404.html;
    error_page 500 /500.html;
}
```

### Step-by-Step Parsing

**Step 1: Read the file**
- Open `config.conf`
- Read it line by line into a string or vector of strings

**Step 2: Tokenize**
- Split into tokens: `server`, `{`, `listen`, `8080`, `;`, etc.
- Ignore comments (`#`) and empty lines

**Step 3: Build Data Structures**

This is where you store everything. Here's the mental model:

```cpp
struct LocationConfig {
    std::string                 path;              // "/uploads"
    std::vector<std::string>    allow_methods;     // ["GET", "POST"]
    std::string                 root;              // "/var/www/html"
    std::string                 index;             // "index.html"
    std::string                 upload_path;       // "/var/www/uploads"
    std::string                 cgi_extension;     // ".py"
    std::string                 cgi_path;          // "/usr/bin/python3"
    bool                        autoindex;         // false
    std::map<int, std::string>  error_pages;       // {404: "/404.html"}
};

struct ServerConfig {
    int                         listen_port;       // 8080
    std::string                 host;              // "0.0.0.0"
    std::string                 server_name;       // "localhost"
    std::string                 root;              // "/var/www/html"
    size_t                      client_max_body_size; // 1048576 (1MB)
    std::vector<LocationConfig> locations;         // [/, /uploads, /cgi-bin]
    std::map<int, std::string>  error_pages;       // inherited + merged
};
```

**Step 4: Validation**
- Is the port valid (1-65535)?
- Does the root path exist?
- Are the methods valid?
- If something is wrong → print error and exit **before** creating any sockets

> **Think:** "Before I open my restaurant, I check the blueprint. If the blueprint says the kitchen is in the bathroom, I stop and fix it. I don't start building."

---

## 3. Server Initialization: Opening the Doors

### From Config to Sockets

Now that you have your `ServerConfig` objects, you create the actual network endpoints.

```cpp
class Server {
public:
    std::vector<ServerConfig> configs;      // From parsing
    std::vector<int>          listen_fds;   // One socket per unique port

    void initialize() {
        for (const ServerConfig& cfg : configs) {
            // Create socket for this server block
            int fd = socket(AF_INET, SOCK_STREAM, 0);

            // Allow port reuse (critical for testing!)
            int opt = 1;
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            // Set NON-BLOCKING (required for select/poll/epoll)
            fcntl(fd, F_SETFL, O_NONBLOCK);

            // Bind to IP:Port
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(cfg.listen_port);
            addr.sin_addr.s_addr = inet_addr(cfg.host.c_str());
            bind(fd, (struct sockaddr*)&addr, sizeof(addr));

            // Start listening
            listen(fd, 128);

            listen_fds.push_back(fd);
        }
    }
};
```

### Key Insight: One Socket Can Serve Multiple Server Names

Notice: you create sockets based on **IP:Port combinations**, not `server_name`.

If you have:
```nginx
server { listen 8080; server_name example.com; }
server { listen 8080; server_name test.com; }
```

You only create **ONE socket** on port 8080. Both server blocks share it. Later, when a request arrives, you use the `Host` header to decide which `server{}` block handles it.

> **Think:** "One front door (port 8080) leads into a lobby. Depending on which name the customer mentions (`Host: example.com` vs `Host: test.com`), I seat them in different dining rooms."

---

## 4. Request Handling Flow: The Customer Arrives

### Step-by-Step When a Client Connects

```
┌─────────────┐     TCP SYN      ┌─────────────┐
│   Browser   │ ───────────────► │  Server OS  │
│             │                  │  (Kernel)   │
└─────────────┘                  └──────┬──────┘
                                        │
                              accept() wakes up
                                        ↓
                              ┌─────────────────┐
                              │  New client_fd  │
                              │  (e.g., fd = 5) │
                              └────────┬────────┘
                                       ↓
                              ┌─────────────────┐
                              │  Add to select  │
                              │  monitoring set │
                              └────────┬────────┘
                                       ↓
                              ┌─────────────────┐
                              │  Wait for data  │
                              │  (select/poll)  │
                              └────────┬────────┘
                                       ↓
┌─────────────┐     HTTP GET     ┌─────────────┐
│   Browser   │ ───────────────► │  Kernel     │
│             │                  │  buffers    │
└─────────────┘                  └──────┬──────┘
                                        │
                              recv(client_fd) reads it
                                        ↓
                              ┌─────────────────┐
                              │  HTTP Parser    │
                              │  breaks request │
                              │  into pieces    │
                              └────────┬────────┘
                                       ↓
                              ┌─────────────────┐
                              │  Router picks   │
                              │  Server &       │
                              │  Location       │
                              └────────┬────────┘
                                       ↓
                              ┌─────────────────┐
                              │  Response       │
                              │  Builder        │
                              └─────────────────┘
```

### The HTTP Parser: Breaking Down the Request

When `recv()` gives you raw bytes, they look like this:

```
GET /index.html HTTP/1.1

Host: localhost:8080

User-Agent: Mozilla/5.0

Accept: text/html



```

Your parser splits this into:

```cpp
struct HttpRequest {
    std::string              method;       // "GET"
    std::string              path;         // "/index.html"
    std::string              version;      // "HTTP/1.1"
    std::string              host;         // "localhost:8080"
    std::map<std::string, std::string> headers;  // {"User-Agent": "...", ...}
    std::string              body;         // "" (empty for GET)
    bool                     is_complete;  // Did we get the full request?
};
```

**Parsing strategy:**
1. Read until you find `

` (end of headers)
2. Split the first line by spaces → method, path, version
3. Split each header line by `:` → key, value
4. If `Content-Length` header exists, read that many more bytes for the body

### The Router: Choosing Server and Location

This is the **brain** of your server. You must answer two questions:

**Question 1: Which `server{}` block?**
- Check the `Host` header from the request
- Match against `server_name` in your configs
- If no match → use the **first** server block for that port (default server)

```cpp
ServerConfig* findServer(const HttpRequest& req, int port) {
    for (ServerConfig& cfg : configs) {
        if (cfg.listen_port == port && cfg.server_name == req.host)
            return &cfg;
    }
    // Return default server for this port
    return getDefaultServer(port);
}
```

**Question 2: Which `location{}` block?**
- Take the request path (`/index.html`)
- Find the **longest matching** location prefix

```nginx
location /        { ... }  // matches everything
location /uploads { ... }  // matches /uploads, /uploads/file.txt
location /api     { ... }  // matches /api, /api/users
```

Example:
- Request: `GET /uploads/photo.jpg`
- Matches `/uploads` (longest match), NOT `/`

```cpp
LocationConfig* findLocation(const std::string& path, ServerConfig* server) {
    LocationConfig* best_match = NULL;
    size_t best_len = 0;

    for (LocationConfig& loc : server->locations) {
        if (path.find(loc.path) == 0 && loc.path.length() > best_len) {
            best_match = &loc;
            best_len = loc.path.length();
        }
    }
    return best_match;
}
```

> **Think:** "The customer says they're here for 'John's party' (`Host: example.com`). I direct them to the correct dining room. Then they ask for 'the seafood section' (`/uploads`). I direct them to that specific area of the menu."

---

## 5. Response Phase: Building the Reply

### Decision Tree

Once you have the right `LocationConfig`, you decide what to do:

```
                    ┌─────────────────┐
                    │  Method allowed?│
                    └────────┬────────┘
                             ↓
              ┌──────────────┴──────────────┐
              NO                            YES
              ↓                             ↓
    Return 405 Method Not Allowed    ┌──────────────┐
                                     │  Path exists?│
                                     └──────┬───────┘
                                            ↓
                              ┌─────────────┴─────────────┐
                              NO                           YES
                              ↓                             ↓
                    Return 404 Not Found           ┌──────────────┐
                                                   │ Is it a CGI? │
                                                   └──────┬───────┘
                                                          ↓
                                            ┌─────────────┴─────────────┐
                                            YES                          NO
                                            ↓                             ↓
                                    ┌──────────────┐            ┌──────────────┐
                                    │ Execute CGI  │            │ Serve static │
                                    │  script      │            │  file        │
                                    │  (fork+exec) │            │  (open+read) │
                                    └──────────────┘            └──────────────┘
```

### Static File Response

```cpp
std::string buildStaticResponse(const std::string& path, LocationConfig* loc) {
    std::string full_path = loc->root + path;  // "/var/www/html" + "/index.html"

    // Check if file exists and is readable
    if (access(full_path.c_str(), R_OK) != 0) {
        return buildErrorResponse(404, loc);
    }

    // Read file content
    std::string body = readFile(full_path);

    // Build HTTP response
    std::ostringstream response;
    response << "HTTP/1.1 200 OK
";
    response << "Content-Length: " << body.length() << "
";
    response << "Content-Type: " << getMimeType(full_path) << "
";
    response << "Connection: close
";
    response << "
";
    response << body;

    return response.str();
}
```

### CGI (Dynamic Content) — Basic Explanation

CGI = Common Gateway Interface. Instead of sending a file, you **run a program** and send its output.

```cpp
// Simplified CGI flow
void handleCGI(const HttpRequest& req, LocationConfig* loc) {
    int pipe_fd[2];
    pipe(pipe_fd);

    pid_t pid = fork();
    if (pid == 0) {
        // Child process: run the script
        dup2(pipe_fd[1], STDOUT_FILENO);  // Script output goes to pipe
        close(pipe_fd[0]);

        char* args[] = {(char*)loc->cgi_path.c_str(), (char*)script_path.c_str(), NULL};
        execve(loc->cgi_path.c_str(), args, envp);
    } else {
        // Parent process: read script output
        close(pipe_fd[1]);
        char buffer[4096];
        int bytes = read(pipe_fd[0], buffer, sizeof(buffer));
        // buffer now contains the CGI output (HTML, JSON, etc.)
        // Build HTTP response with this as body
    }
}
```

> **Think:** "Static file = I hand them a pre-printed menu. CGI = I run to the kitchen, ask the chef to cook something fresh, and bring back the result."

### Error Responses

If anything fails, you build an error response using the configured error pages:

```cpp
std::string buildErrorResponse(int code, LocationConfig* loc) {
    std::string error_path = loc->error_pages[code];  // e.g., "/404.html"
    if (!error_path.empty()) {
        return buildStaticResponse(error_path, loc);  // Custom error page
    }
    // Fallback: simple HTML
    return "HTTP/1.1 404 Not Found
Content-Length: 9

Not Found";
}
```

---

## 6. Loop / Runtime Behavior: The Heartbeat

### The Main Loop Structure

This is what your `main.cpp` looks like in practice:

```cpp
int main(int argc, char** argv) {
    // ========== STARTUP PHASE ==========
    ConfigParser parser(argv[1]);
    std::vector<ServerConfig> configs = parser.parse();

    Server server(configs);
    server.initialize();  // socket() + bind() + listen() for all ports

    // ========== MAIN LOOP ==========
    while (true) {
        // 1. Wait for activity on ALL sockets
        server.waitForActivity();  // select() / poll() / epoll()

        // 2. Check each socket for what happened
        for (int fd : server.all_fds) {
            if (server.isNewConnection(fd)) {
                // New client knocking → accept()
                server.acceptClient(fd);
            }
            else if (server.hasData(fd)) {
                // Existing client sent data → recv()
                server.handleClient(fd);
            }
        }

        // 3. Clean up closed connections
        server.removeClosedClients();
    }
}
```

### Handling Multiple Connections

Your server must juggle many clients at once. Here's how the state machine works:

```cpp
class Client {
public:
    int         fd;
    std::string read_buffer;      // Data received so far
    std::string write_buffer;     // Response ready to send
    HttpRequest request;          // Parsed request
    bool        request_complete; // Do we have the full request?
    bool        response_ready;   // Is response built?
    bool        should_close;     // Close after sending?
};
```

**The state flow per client:**

```
┌─────────────┐    recv()    ┌─────────────┐    parse()    ┌─────────────┐
│   READING   │ ───────────► │  PARSING    │ ────────────► │  PROCESSING │
│  (getting   │              │  (building  │               │  (routing,  │
│   data)     │              │   request)  │               │   building  │
└─────────────┘              └─────────────┘               │   response) │
                                                           └──────┬──────┘
                                                                  ↓
                                                           ┌─────────────┐
                                                           │   SENDING   │
                                                           │  (send()    │
                                                           │   response) │
                                                           └──────┬──────┘
                                                                  ↓
                                                           ┌─────────────┐
                                                           │   CLOSED    │
                                                           │  (close fd) │
                                                           └─────────────┘
```

### Why Non-Blocking + `select()` Changes Everything

In a **blocking** server:
```cpp
client_fd = accept(server_fd, ...);     // BLOCKS
recv(client_fd, buffer, ...);           // BLOCKS until data arrives
// Client A is slow? Client B waits outside forever.
```

In your **non-blocking + `select()`** server:
```cpp
// Set ALL sockets to non-blocking
fcntl(fd, F_SETFL, O_NONBLOCK);

// The magic: you monitor ALL sockets at once
fd_set read_fds, write_fds;
FD_SET(server_fd, &read_fds);
for (Client* c : clients) {
    FD_SET(c->fd, &read_fds);
    if (c->response_ready) FD_SET(c->fd, &write_fds);
}

select(max_fd + 1, &read_fds, &write_fds, NULL, NULL);
// Returns when ANY socket is ready for reading OR writing
```

**What this means:**
- Client A is taking 10 seconds to send data? **No problem.** `select()` will skip them and handle Client B.
- Client C's response is ready to send while Client D is still sending their request? **No problem.** `select()` tells you C is ready for writing.
- Your server is **never stuck waiting** on one slow client.

> **Think:** "I'm a waiter with 10 tables. Instead of standing at Table 3 waiting for them to decide, I check all tables quickly. If Table 3 isn't ready, I serve Table 5. I never let one slow customer freeze the whole restaurant."

---

## 7. Mental Model: The Restaurant Story

Let me tell you the story of your server from start to finish.

---

### 🏗️ Chapter 1: The Blueprint (Config Parsing)

You want to open a restaurant. Before anything else, you hire an architect who gives you a blueprint (`config.conf`).

The blueprint says:
- **Address:** 8080 Main Street (port 8080)
- **Restaurant name:** "Localhost Café" (`server_name`)
- **Main storage room:** `/var/www/html` (`root`)
- **Special sections:**
  - Regular dining area `/` — serves GET and POST
  - File upload room `/uploads` — POST only
  - Live kitchen `/cgi-bin` — chef cooks fresh meals (CGI)

You read the blueprint carefully. If it says the freezer should be in the bathroom, you throw it away and refuse to build.

---

### 🚪 Chapter 2: Building the Entrance (Server Initialization)

You build the front door (`socket()`). You put up the address sign (`bind()` to port 8080). You unlock it and flip the "Open" sign (`listen()`).

The door is special — it's **non-blocking**. People can knock, but if no one is there, you don't stand frozen staring at the door. You immediately turn around and do other work.

---

### ⏳ Chapter 3: The Waiting Game (Main Loop)

You sit at your reception desk with a **bell system** (`select()`). You have:
- One bell for the front door (new customers)
- One bell for every occupied table (existing customers needing service)

You fall asleep. The bells will wake you up.

---

### 🔔 Chapter 4: A Customer Arrives (Accept)

*Ding!* The front door bell rings.

A customer walks in. You don't talk to them at the door! You create a **new private table** just for them (`accept()` → new `client_fd`). You give them a table number (file descriptor 5). You add their table bell to your bell system.

The front door stays open. You go back to sleep.

---

### 🍽️ Chapter 5: Taking the Order (Recv + Parse)

*Ding!* Table 5's bell rings.

You walk over. The customer hands you a slip of paper (`recv()`):

```
GET /index.html HTTP/1.1
Host: localhost:8080
```

You read it and understand:
- They want to **GET** something
- They want `/index.html`
- They're here for "localhost:8080"

You check your blueprint. "localhost:8080" matches your main dining room. `/index.html` matches the regular dining area `/`. GET is allowed there. Good.

---

### 👨‍🍳 Chapter 6: Preparing the Meal (Building Response)

You walk to the storage room (`/var/www/html`) and look for `index.html`. You find it! You read its contents.

You prepare a tray with:
- A label: "HTTP/1.1 200 OK"
- A note about how big it is: "Content-Length: 1234"
- A note about what it is: "Content-Type: text/html"
- The actual food: the HTML file contents

---

### 🏃 Chapter 7: Serving (Send)

You carry the tray back to Table 5 and place it down (`send()`). The customer starts eating (the browser renders the page).

You ask: "Anything else?" They say no. You clear the table and say goodbye (`close()`). Table 5 is now free.

---

### 🔄 Chapter 8: The Cycle Continues

You go back to your desk. The bells are still there, monitoring all tables and the front door. You fall asleep again, waiting for the next *ding!*

---

## 8. Simple Example Walkthrough

Let's walk through a **complete real example** step by step.

### The Config File

```nginx
# config.conf
server {
    listen 8080;
    server_name localhost;
    root ./www;
    index index.html;
    client_max_body_size 1M;

    location / {
        allow_methods GET;
    }

    location /uploads {
        allow_methods POST;
        upload_path ./www/uploads;
    }

    error_page 404 ./www/404.html;
}
```

### Directory Structure

```
./webserv
├── config.conf
├── webserv           (your binary)
└── www/
    ├── index.html    (contains: "<h1>Hello</h1>")
    ├── 404.html      (contains: "<h1>Not Found</h1>")
    └── uploads/      (empty folder)
```

### The Request

User opens browser and types: `http://localhost:8080/index.html`

Browser sends:
```
GET /index.html HTTP/1.1

Host: localhost:8080

User-Agent: Mozilla/5.0

Accept: text/html



```

---

### Step-by-Step Internal Flow

| Step | What Happens | Code/Action |
|------|-------------|-------------|
| **1** | `./webserv config.conf` starts | `main()` begins |
| **2** | Config parser reads `config.conf` | `ConfigParser::parse()` |
| **3** | Creates `ServerConfig` object | `listen_port=8080`, `server_name="localhost"`, `root="./www"` |
| **4** | Creates `LocationConfig` for `/` | `allow_methods=["GET"]` |
| **5** | Creates `LocationConfig` for `/uploads` | `allow_methods=["POST"]`, `upload_path="./www/uploads"` |
| **6** | Server calls `socket()` | `server_fd = 3` |
| **7** | Server calls `setsockopt(SO_REUSEADDR)` | Allows quick restart |
| **8** | Server sets non-blocking | `fcntl(3, F_SETFL, O_NONBLOCK)` |
| **9** | Server calls `bind()` | Binds to `0.0.0.0:8080` |
| **10** | Server calls `listen()` | Queue of 128 pending connections |
| **11** | Main loop starts | `while (true) { select(...); }` |
| **12** | Browser does DNS lookup | `localhost` → `127.0.0.1` |
| **13** | Browser calls `connect()` to port 8080 | TCP handshake happens |
| **14** | `select()` detects activity on `server_fd` | `FD_ISSET(3, &read_fds)` is true |
| **15** | Server calls `accept()` | Returns `client_fd = 4` |
| **16** | Server adds `client_fd=4` to `select()` set | Now monitoring fd 3 and fd 4 |
| **17** | Browser sends HTTP request | Data arrives in kernel buffer |
| **18** | `select()` detects activity on fd 4 | `FD_ISSET(4, &read_fds)` is true |
| **19** | Server calls `recv(4, buffer, 4096, 0)` | Reads `GET /index.html...` into buffer |
| **20** | HTTP parser splits the request | `method="GET"`, `path="/index.html"`, `host="localhost:8080"` |
| **21** | Router finds server block | Matches `server_name "localhost"` on port 8080 |
| **22** | Router finds location block | Longest match: `/` (not `/uploads`) |
| **23** | Checks if GET is allowed | Yes, `allow_methods` contains GET |
| **24** | Builds full file path | `./www` + `/index.html` = `./www/index.html` |
| **25** | Checks if file exists | `access("./www/index.html", R_OK)` returns 0 (success) |
| **26** | Reads file content | `"<h1>Hello</h1>"` |
| **27** | Builds HTTP response | `"HTTP/1.1 200 OK
Content-Length: 14
Content-Type: text/html

<h1>Hello</h1>"` |
| **28** | Server calls `send(4, response, ...)` | Data copied to kernel, sent to browser |
| **29** | Browser receives response | Parses HTTP, renders HTML |
| **30** | Server calls `close(4)` | Connection terminated |
| **31** | Server removes fd 4 from `select()` set | Back to monitoring only fd 3 |
| **32** | Browser displays "Hello" to user | Page is visible! |

---

### What If the File Didn't Exist?

If the user requested `GET /nonexistent.html`:

| Step | What Changes |
|------|-------------|
| **24** | Path becomes `./www/nonexistent.html` |
| **25** | `access()` fails (returns -1) |
| **26** | Server looks up error page: `error_page 404 ./www/404.html` |
| **27** | Reads `./www/404.html` instead |
| **28** | Response becomes: `HTTP/1.1 404 Not Found
...` + 404.html content |

---

### What If They POST to `/uploads`?

If the user sends `POST /uploads/file.txt` with body `Hello World`:

| Step | What Happens |
|------|-------------|
| **22** | Router matches `/uploads` (longer than `/`) |
| **23** | POST is allowed |
| **24** | Server saves body to `./www/uploads/file.txt` |
| **27** | Response: `HTTP/1.1 201 Created
...` |

---

## Summary: Your Implementation Checklist

Use this as your project roadmap:

```
□ Phase 1: Config Parser
  □ Read file line by line
  □ Tokenize (handle braces, semicolons, comments)
  □ Build ServerConfig and LocationConfig structs
  □ Validate all values

□ Phase 2: Socket Setup
  □ Create socket per unique IP:port
  □ Set SO_REUSEADDR
  □ Set NON-BLOCKING
  □ bind() and listen()

□ Phase 3: Main Loop (select/poll/epoll)
  □ Initialize fd_set with all listen sockets
  □ Loop: call select()
  □ If listen socket ready → accept() → add new client
  □ If client socket ready → recv() → parse

□ Phase 4: HTTP Parser
  □ Read until 

 (end of headers)
  □ Parse request line: METHOD PATH VERSION
  □ Parse headers into map
  □ Read body if Content-Length present

□ Phase 5: Router
  □ Match Host header to server_name
  □ Match path to longest location prefix
  □ Check if method is allowed

□ Phase 6: Response Builder
  □ Static files: open, read, build HTTP response
  □ CGI: fork, exec, capture output
  □ Errors: use custom error pages or defaults
  □ Set correct Content-Type, Content-Length

□ Phase 7: Send and Cleanup
  □ send() response
  □ close() connection
  □ Remove from select set
```

---

**Remember:** Your server is just a loop. Parse config → open doors → wait → accept → read → route → build → send → close → repeat. Everything else is details around this core flow.

Build it piece by piece, test each phase before moving to the next, and you'll have a solid `webserv`. Good luck! 🚀
