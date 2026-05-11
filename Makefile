CXX = c++
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror -Iinc
LDFLAGS =

SRC_DIR = src
REQUEST_RESPONSE_DIR = $(SRC_DIR)/Request_Responce
INC_DIR = inc

# Source files
SRCS = $(REQUEST_RESPONSE_DIR)/Request.cpp \
       $(REQUEST_RESPONSE_DIR)/Request_getset.cpp\

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
