CXX = c++
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror -Iinc -g3
LDFLAGS =

SRC_DIR = src
REQUEST_RESPONSE_DIR = $(SRC_DIR)/Request_Responce
SERVER_DIR = $(SRC_DIR)/Server
MPLEXER_DIR = $(SRC_DIR)/Multiplexer
INC_DIR = inc

# Source files
SRCS = $(SRC_DIR)/main.cpp \
	$(MPLEXER_DIR)/AHandler.cpp \
	$(MPLEXER_DIR)/Client.cpp \
	$(MPLEXER_DIR)/EventLoop.cpp \
	$(MPLEXER_DIR)/Server_Handler.cpp \
	$(REQUEST_RESPONSE_DIR)/Request.cpp \
	$(REQUEST_RESPONSE_DIR)/Request_getset.cpp \
	$(REQUEST_RESPONSE_DIR)/Response.cpp \
	$(REQUEST_RESPONSE_DIR)/Response_getset.cpp \
	$(SERVER_DIR)/configParser.cpp \
	$(SERVER_DIR)/locationConfig.cpp \
	$(SERVER_DIR)/serverConfig.cpp \
	$(SERVER_DIR)/Server.cpp \
	$(SERVER_DIR)/Router.cpp \
	$(SERVER_DIR)/RouteResult.cpp \
	$(MPLEXER_DIR)/CommoneGatewayInterface.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Executable name
TARGET = webserv

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^
	@echo "✅ Build successful: $(TARGET)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
	@echo "🧹 Cleaned"

fclean:
	rm -rf $(TARGET) $(OBJS)
	@echo "🧹 Fully cleaned"

re: fclean all

.PHONY: all clean fclean re
.SECONDARY: $(OBJS)
