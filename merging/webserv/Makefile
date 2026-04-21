NAME = webserv

CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP
# Uncomment the next line when Person 1 merges CGIHandler
# CXXFLAGS += -DHAVE_CGI
INC = -I./includes

SRCS = \
	srcs/config/Tokenizer.cpp \
	srcs/config/Parser.cpp \
	srcs/config/Parser_helper.cpp \
	srcs/config/server_block.cpp \
	srcs/http/HTTPRequest.cpp \
	srcs/http/RequestParser.cpp \
	srcs/http/ChunkedDecoder.cpp \
	srcs/http/FileUpload.cpp \
	srcs/http/ResponseBuilder.cpp \
	srcs/http/MethodHandler.cpp \
	srcs/http/Router.cpp \
	srcs/http/RouteConfig.cpp \
	main.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INC) -o $(NAME) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS)

fclean: clean
	rm -f $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re
