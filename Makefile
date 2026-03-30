# ==========================================
# ============ Color Definitions ===========
# ==========================================
RED     := \033[0;31m
GRN     := \033[0;32m
YEL     := \033[0;33m
BLUE    := \033[38;2;35;177;239m
RESET   := \033[0m

# ==========================================
# ============ Compiler Settings ===========
# ==========================================
CXX      := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -MMD -MP
RM       := rm -rf
NAME     := webserv

# ==========================================
# ============ Mandatory Paths =============
# ==========================================
MANDO       := mandatory
MANDO_SRC   := $(MANDO)/src
MANDO_INC   := $(MANDO)/include

CORE_PATH := $(MANDO_SRC)/core
CGI_PATH  := $(MANDO_SRC)/cgi

# ==========================================
# ============ Mandatory Sources ===========
# ==========================================
CORE := $(CORE_PATH)/Client.cpp \
        $(CORE_PATH)/EventLoop.cpp \
        $(CORE_PATH)/Server.cpp

CGI := $(CGI_PATH)/CGIHandler.cpp

SRC  := $(CORE) $(CGI) $(MANDO_SRC)/main.cpp
OBJS := $(SRC:.cpp=.o)
DEPS := $(OBJS:.o=.d)

# ==========================================
# =============== Rules ====================
# ==========================================
all: $(NAME)

$(NAME): $(OBJS)
	@printf "$(GRN)  Linking $(NAME)...$(RESET)\n"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@printf "$(GRN)  Successfully built $(NAME)$(RESET)\n"

# Compile .cpp → .o
%.o: %.cpp
	@printf "$(BLUE)  Compiling $<...$(RESET)\n"
	@$(CXX) $(CXXFLAGS) -I$(MANDO_INC) -c $< -o $@

clean:
	@printf "$(RED)  Cleaning object files...$(RESET)\n"
	@$(RM) $(OBJS) $(DEPS)
	@printf "$(GRN)  Object files removed$(RESET)\n"

fclean: clean
	@printf "$(RED)  Removing executable...$(RESET)\n"
	@$(RM) $(NAME)
	@printf "$(GRN)  Full cleanup complete$(RESET)\n"

re: fclean all
	@printf "$(YEL)  Rebuilding project...$(RESET)\n"

# Include auto-generated dependency files
-include $(DEPS)

.PHONY: all clean fclean re