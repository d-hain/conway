# (my implementation of) Conway's Game of Life

This is my simple implementation of Conway's Game of Life.
It is able to be run either in the terminal or a graphical window using [Raylib](https://raylib.com).
There are many command line options and at least one extra color scheme to choose.

Controls are "q" for quitting, "space" for manually stepping through the
simulation when using --step-manually and the mouse for interacting with the
graphical window.

## Running the game

Just run `./conway` to see the terminal version or `./conway --raylib` for the graphical one.
For more information on all the options use the `-h` or `--help` flag.

## Building from source

I am building via Nix but it should also work just with the Makefile or just gcc/clang if you manually compile or download raylib and link it.

To build the release build with optimizations run:
```shell
nix build
```
and to run it use:
```shell
./result/bin/conway
```


To build the debug build run:
```shell
nix build .#conway-debug
```
and to run it use:
```shell
./result/bin/conway-debug
```

To build and run it using Nix use something like the following:
```shell
nix run .#conway -- --grid-rows 100 --grid-cols 100 --step-manually --glider-gun --raylib --color-scheme hacker
```

## Thank yous

- [Conway's Game of Life article on Wikipedia](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) for the 4 rules and Gosper's glider gun.
- [Raylib](https://raylib.com) for making opening and drawing in a window simple.
