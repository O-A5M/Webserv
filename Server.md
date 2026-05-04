main.cpp
   │
   │  ConfigParser parser("config.conf")
   │  vector<ServerConfig> configs = parser.getServers()
   │
   │  Server server(configs)
   │  server.run()
   │
   ▼
Server
   │
   │  setupSockets()
   │    → for each ServerConfig → createSocket() → bind → listen
   │    → add each server_fd to _pollFds
   │
   │  run()  ← forever loop
   │    → poll(_pollFds)
   │    → if server_fd active → acceptClient()
   │         → creates Client object
   │         → adds client_fd to _pollFds
   │    → if client_fd active → handleClient()
   │         → client.readData()
   │         → if request ready → Person 2 builds response
   │         → client.setResponse(response)
   │         → client.writeData()
   │         → if done → removeClient()
   │
   ▼
Client
   │
   │  readData()   → recv() into _readBuffer
   │  writeData()  → send() from _writeBuffer
   │  isRequestReady() → checks for "\r\n\r\n"
   │
   ▼
Person 2 (HTTP Handler) ← gets raw request from client
                        → returns response string to server