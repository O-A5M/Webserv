#include "../../inc/Client.hpp"
#include "../../inc/Response.hpp"


ClientHandler::ClientHandler(int fd, ServerConfig &config
    , EventLoop &loop
    , const struct sockaddr_in &addr, socklen_t addrLen)
        : AHandler(fd, config, loop)
       /* , addr(addr)
        , addrLen(addrLen) */{
		(void) addrLen; // To avoid unused parameter warning
		(void) addr; // To avoid unused parameter warning
    loop.AddHandler(this, EPOLLIN);
}


LocationConfig getTemporaryLocation() {
    LocationConfig fake_loc;
    
    // 1. Basic Path & Root
    fake_loc.path = "/";
    fake_loc.root = "www/html";
    
    // 2. Index files (Using push_back for C++98 std::vector)
    fake_loc.index.push_back("index.html");
    
    // 3. Allowed Methods (We allow GET and POST, but leave out DELETE to test your 405 error)
    fake_loc.allow_methods.push_back("GET");
    fake_loc.allow_methods.push_back("POST");
    
    // 4. Limits & Behaviors
    fake_loc.autoindex = false;
    fake_loc.client_max_body_size = 10485760; // 10 Megabytes in bytes
    
    // 5. CGI & Uploads (Leaving CGI blank for now, setting upload folder)
    fake_loc.cgi_extension = "";
    fake_loc.cgi_path = "";
    fake_loc.upload_store = "www/html/uploads";
    
    // 6. Redirects
    fake_loc.redirect = "";
    
    return fake_loc;
}

ClientHandler::~ClientHandler(void) {}

void    ClientHandler::OnRead(void) {
    char    buff[4096];
    ssize_t nread = recv(fd, buff, sizeof(buff), 0);

    if (nread == 0) {
        OnError();
        return;
    }
    if (nread == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cerr << "ClientHandler::OnRead() error: "
            << strerror(errno) << std::endl;
        OnError();
        return ;
    }
    readBuf.append(buff, nread);
    int parse_status = this->req.parse_request(readBuf);
    std::cout << "s " << parse_status << std::endl;
    // if (parse_status == 0)
    // {
    //     std::cout << "not complete" << std::endl;
    //     return ; 
    // }
    Response res;
    if (parse_status == -1) {
        // Status -1: BAD REQUEST (e.g., malformed headers).
        res.setStatusCode(400);
        res.setReasonPhrase("Bad Request");
        // Optional: generate a generic 400 HTML body here if you want
        res.buildRawResponse();
    }
    // } else if (parse_status == 1){
        ServerConfig &my_config = this->GetServerConf();
        LocationConfig matched_location = getTemporaryLocation();
        res.handleRequest(this->req, matched_location , my_config);
    // }
    this->writeBuf = res.getRawResponse();
    if (!writeBuf.empty())
        EnableWrite();
    this->req.clear();
}

void    ClientHandler::OnWrite(void) {
    while (!writeBuf.empty()) {
        ssize_t nwrite = send(fd, writeBuf.data(), writeBuf.size(), 0);
        if (nwrite == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            std::cerr << "ClientHandler: OnWrite() error: "
                << strerror(errno) << std::endl;
            OnError();
            return;
        }
        writeBuf.erase(0, nwrite);
    }
    writeBuf.clear();
    DisableWrite();
}

void    ClientHandler::OnError(void) {
    loop.RemoveHandler(this);
    delete this;
}
