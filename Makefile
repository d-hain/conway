NAME = conways-game-of-life

CC = gcc
CC_FLAGS = -std=c23
CC_FLAGS += -Wall -Werror -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function

.PHONY: run-debug run-release

run-debug: target/$(NAME)
	./target/$(NAME)

run-release: target/release/$(NAME)
	./target/release/$(NAME)

target/$(NAME): src/main.c target
	$(CC) src/main.c -o target/$(NAME) $(CC_FLAGS) -ggdb
	chmod u+x ./target/$(NAME)

target/release/$(NAME): src/main.c target/release
	$(CC) src/main.c -o target/release/$(NAME) $(CC_FLAGS) -O3
	chmod u+x ./target/release/$(NAME)

target:
	mkdir -p target

target/release:
	mkdir -p target/release
