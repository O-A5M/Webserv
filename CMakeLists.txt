cmake_minimum_required(VERSION 4.2)
project(Webserv)

set(CMAKE_CXX_STANDARD 14)

include_directories(inc)
include_directories(src/Client)
include_directories(src/Event_Loop)
include_directories(src/Request_Responce)
include_directories(src/Router/CGI)

add_executable(Webserv
    inc/include.hpp
    src/Client/Client.cpp
    src/Client/Client.hpp
    src/Event_Loop/event_loop.cpp
    src/Event_Loop/EventLoop.cpp
    src/Event_Loop/EventLoop.hpp
    src/Request_Responce/Request.cpp
    src/Request_Responce/Request.hpp
    src/Request_Responce/Responce.hpp
    src/Request_Responce/Response.cpp
    src/Router/CGI/CommonGatewayInterface.cpp
    src/Router/CGI/CommonGatwayInterface.hpp)
