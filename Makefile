CXX = c++
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror -Iinc
LDFLAGS = 

SRC_DIR = src
SERVER_DIR = $(SRC_DIR)/Server
INC_DIR = inc

# Source files
SRCS = $(SERVER_DIR)/configParser.cpp \
       $(SERVER_DIR)/serverConfig.cpp \
       $(SERVER_DIR)/locationConfig.cpp \
       $(SRC_DIR)/main.cpp \
	   $(SERVER_DIR)/Server.cpp 

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

re: clean all

.PHONY: all clean re