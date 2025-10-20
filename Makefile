NAME = conway

CC = gcc
CC_FLAGS = -std=c23
CC_FLAGS += -Wall -Werror -Wextra -Wpedantic -pedantic
CC_FLAGS += -Wunused -Wunused-parameter -Wunused-but-set-parameter -Wunused-variable -Wunused-but-set-variable
CC_FLAGS += -Wunused-const-variable -Wunused-function -Wunused-label -Wunused-local-typedefs -Wunused-macros -Wunused-value
CC_FLAGS += -Wuse-after-free -Wuseless-cast -Wunused-result
CC_FLAGS += -fanalyzer -fanalyzer-fine-grained
# -isystem makes it so warnings are ignored for Raylib
CC_LINK_FLAGS = -isystem include -L lib -l:libraylib.a -lm -lglfw

debug/$(NAME): src/main.c debug
	$(CC) src/main.c -o debug/$(NAME)-debug $(CC_FLAGS) $(CC_LINK_FLAGS) -ggdb
	chmod u+x ./debug/$(NAME)-debug

release/$(NAME): src/main.c release
	$(CC) src/main.c -o release/$(NAME) $(CC_FLAGS) $(CC_LINK_FLAGS) -O3
	chmod u+x ./release/$(NAME)

debug:
	mkdir -p debug

release:
	mkdir -p release
