NAME = conway

CC = gcc
CC_FLAGS = -std=c23
CC_FLAGS += -Wall -Werror -Wextra -Wpedantic -w include/raylib.h
CC_LINK_FLAGS = -I include -L lib -l:libraylib.a -lm -lglfw

.PHONY: run-debug run-release

run-debug: target/$(NAME)
	./target/$(NAME)

run-release: target/release/$(NAME)
	./target/release/$(NAME)

target/$(NAME): src/main.c target
	$(CC) src/main.c -o target/$(NAME) $(CC_FLAGS) $(CC_LINK_FLAGS) -ggdb
	chmod u+x ./target/$(NAME)

target/release/$(NAME): src/main.c target/release
	$(CC) src/main.c -o target/release/$(NAME) $(CC_FLAGS) $(CC_LINK_FLAGS) -O3
	chmod u+x ./target/release/$(NAME)

target:
	mkdir -p target

target/release:
	mkdir -p target/release
