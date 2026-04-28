# ------------ Color Definitions ------------
RED		:=	\033[0;31m
GRN		:=	\033[0;32m
YEL		:=	\033[0;33m
BLUE	:=	\033[38;2;35;177;239m
RESET	:=	\033[0m

# ------------ Compiler Settings ------------
CXX			:=	c++
CXXFLAGS	:=	-Wall -Wextra -Werror -std=c++98 -MMD -MP
RM			:=	rm -rf
NAME		:=	webserv
VAL			:=	valgrind --leak-check=full --track-fds=yes --show-reachable=yes

# ------------ Paths ------------
SRC_PATH	:=	src
INC_PATH	:=	include
OBJ_PATH	:=	.build

CORE_PATH	:=	$(SRC_PATH)/core
CGI_PATH	:=	$(SRC_PATH)/cgi
CONFIG_PATH	:=	$(SRC_PATH)/config
HTTP_PATH	:=	$(SRC_PATH)/http
LOGGER_PATH	:=	$(SRC_PATH)/logger

# ------------ Sources ------------
CORE	:=	$(CORE_PATH)/Client.cpp \
			$(CORE_PATH)/EventLoop.cpp \
			$(CORE_PATH)/Server.cpp

CGI		:=	$(CGI_PATH)/CGIHandler.cpp

CONFIG	:=	$(CONFIG_PATH)/Tokenizer.cpp \
			$(CONFIG_PATH)/Parser.cpp \
			$(CONFIG_PATH)/Parser_helper.cpp \
			$(CONFIG_PATH)/Server_block.cpp

HTTP	:=	$(HTTP_PATH)/HTTPRequest.cpp \
			$(HTTP_PATH)/RequestParser.cpp \
			$(HTTP_PATH)/ChunkedDecoder.cpp \
			$(HTTP_PATH)/FileUpload.cpp \
			$(HTTP_PATH)/ResponseBuilder.cpp \
			$(HTTP_PATH)/MethodHandler.cpp \
			$(HTTP_PATH)/Router.cpp \
			$(HTTP_PATH)/RouteConfig.cpp

LOGGER	:=	$(LOGGER_PATH)/Logger.cpp \

SRC		:=	$(CORE) $(CGI) $(CONFIG) $(HTTP) $(LOGGER) main.cpp
OBJS	:=	$(addprefix $(OBJ_PATH)/, $(SRC:.cpp=.o))
DEPS	:=	$(OBJS:.o=.d)

# ------------ Rules ------------
all: $(NAME)

valgrind: $(NAME)
	@printf "$(GRN)  Execute with VALGRIND$(RESET)\n"
	@$(VAL) ./$(NAME)

$(NAME): $(OBJS)
	@printf "$(GRN)  Linking $(NAME)...$(RESET)\n"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@printf "$(GRN)  Successfully built $(NAME)$(RESET)\n"

$(OBJ_PATH)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@printf "$(BLUE)  Compiling $<...$(RESET)\n"
	@$(CXX) $(CXXFLAGS) -I$(INC_PATH) -c $< -o $@

clean:
	@printf "$(RED)  Cleaning object files...$(RESET)\n"
	@$(RM) $(OBJ_PATH)
	@printf "$(GRN)  Object files removed$(RESET)\n"

fclean: clean
	@printf "$(RED)  Removing executable...$(RESET)\n"
	@$(RM) $(NAME)
	@printf "$(GRN)  Full cleanup complete$(RESET)\n"

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re valgrind