

CPP = c++
CPP_FLAGS = -Wall -Wextra -Werror

SRCS = ./srcs/config/Parser_helper.cpp ./srcs/config/Parser.cpp ./srcs/config/server_block.cpp ./srcs/config/Tokenizer.cpp ./srcs/main.cpp \
		./srcs/http/Router.cpp 
OBJS = $(SRCS:.cpp=.o)

NAME = webserve

INCLUDES = ./includes/config/location_block.hpp  ./includes/config/server_block.hpp ./includes/config/Tokenizer.hpp ./includes/config/Parser.hpp \
		    ./includes/http/Request.hpp ./includes/http/Router.hpp
all: $(NAME)

$(NAME): $(OBJS)
	$(CPP) $(CPP_FLAGS) -o $(NAME) $(OBJS)

%.o:%.cpp $(INCLUDES)
	$(CPP) $(CPP_FLAGS) -c $< -o $@ 

clean: 
	rm $(OBJS)

fclean: clean
	rm $(NAME)

re: fclean all

.PHONY: re fclean clean all


