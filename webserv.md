*This project has been created as part of the 42 curriculum by aben-kar.*

# webserv

## Description
webserv is a configurable HTTP server written in C++98 for the 42 curriculum. It reads a custom configuration file, starts one or more listening sockets, and serves HTTP requests through an event-driven loop.

The project includes support for static file delivery, directory listing, file uploads, redirections, CGI execution, and route-specific settings such as allowed methods, index files, client body size limits, and error pages.

## Instructions
### Build
The project is built with the provided Makefile:

```bash
make
```

### Run
Launch the server with a valid configuration file:

```bash
./webserv Config/configFile.conf
```

The server expects exactly one argument: the path to the configuration file.

### Useful targets
```bash
make clean
make fclean
make re
```

### Stop the server
Use `Ctrl+C` to stop the process cleanly.

## Resources
### References
- MDN Web Docs: HTTP overview and request/response concepts
- RFC 9110: HTTP Semantics
- RFC 9112: HTTP/1.1
- RFC 3875: Common Gateway Interface (CGI) 1.1
- Linux man pages for `socket`, `bind`, `listen`, `accept`, `poll`, `select`, `read`, and `write`

### AI usage
AI was used to draft and structure this README, summarize the project behavior from the source code, and organize the build and run instructions. The technical details were checked against the repository's Makefile, entry point, and configuration parser.