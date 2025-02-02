#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sysexits.h>
#include <termios.h>
#include <pthread.h>
#include <sys/param.h>
#include <sys/time.h>

#define ARR_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

#define PRINT_ERR(fmt, ...) fprintf(stderr, "[ERROR] %s:%d: " fmt, __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)

#define CELL_ARRAY_BOUNDS_CHECK(grid, y, x)                                      \
    if (y > grid.height) {                                                       \
        PRINT_ERR("Cell Array index out of range! Y Coordinate is too big.\n");  \
        exit(EX_DATAERR);                                                        \
    }                                                                            \
    if (x > grid.width) {                                                        \
        PRINT_ERR("Cell Array index out of range! X Coordinate is too big.\n");  \
        exit(EX_DATAERR);                                                        \
    }

#define CELL_ARRAY_PTR_BOUNDS_CHECK(grid, y, x)                                  \
    if (y > grid->height) {                                                      \
        PRINT_ERR("Cell Array index out of range! Y Coordinate is too big.\n");  \
        exit(EX_DATAERR);                                                        \
    }                                                                            \
    if (x > grid->width) {                                                       \
        PRINT_ERR("Cell Array index out of range! X Coordinate is too big.\n");  \
        exit(EX_DATAERR);                                                        \
    }

/**
*  A 2d array of cells.
*/
typedef struct {
    bool **cells;
    size_t width;
    size_t height;
} Cell_Array_2d;

bool cell_array_get(Cell_Array_2d cell_array, size_t y, size_t x) {
    CELL_ARRAY_BOUNDS_CHECK(cell_array, y, x);

    return cell_array.cells[y][x];
}

void cell_array_set(Cell_Array_2d *cell_array, size_t y, size_t x, bool value) {
    CELL_ARRAY_PTR_BOUNDS_CHECK(cell_array, y, x);

    cell_array->cells[y][x] = value;
}

Cell_Array_2d cell_array_init(size_t width, size_t height) {
    Cell_Array_2d cell_array = {
        .cells = malloc(sizeof(bool*) * height),
        .width = width,
        .height = height,
    };
    for (size_t idx = 0; idx < height; idx++) {
        cell_array.cells[idx] = malloc(sizeof(cell_array.cells[0]) * width);
    }

    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            cell_array.cells[y][x] = false;
        }
    }

    return cell_array;
}

void cell_array_free(Cell_Array_2d cell_array) {
    for (size_t idx = 0; idx < cell_array.height; idx++) {
        free(cell_array.cells[idx]);
    }
    free(cell_array.cells);
    cell_array.width = 0;
    cell_array.height = 0;
}

uint8_t cell_array_alive_neighbor_count(Cell_Array_2d cell_array, size_t y, size_t x) {
    CELL_ARRAY_BOUNDS_CHECK(cell_array, y, x);

    int32_t indices[8][2] = {
        {y, x - 1}, // Left
        {y, x + 1}, // Right
        {y - 1, x}, // Top
        {y - 1, x - 1}, // Left-Top
        {y - 1, x + 1}, // Right-Top
        {y + 1, x}, // Bottom
        {y + 1, x - 1}, // Left-Bottom
        {y + 1, x + 1}, // Right-Bottom
    };

    uint8_t alive_neighbor_count = 0;
    for (size_t i = 0; i < ARR_LEN(indices); i++) {
        // Validate indices
        int32_t y_idx = indices[i][0];
        int32_t x_idx = indices[i][1];

        if (y_idx < 0 || x_idx < 0) {
            continue;
        }
        size_t y_idx_unsigned = y_idx;
        size_t x_idx_unsigned = x_idx;

        if (y_idx_unsigned >= cell_array.height || x_idx_unsigned >= cell_array.width) {
            continue;
        }

        // Index is valid -> check if alive
        if (cell_array_get(cell_array, y_idx_unsigned, x_idx_unsigned) == true) {
            alive_neighbor_count++;
        }
    }

    return alive_neighbor_count;
}

void cell_array_print(Cell_Array_2d cell_array, bool print_neighbors) {
    for (size_t y = 0; y < cell_array.height; y++) {
        for (size_t x = 0; x < cell_array.width; x++) {
            if (print_neighbors) {
                uint8_t alive_neighbor_count = cell_array_alive_neighbor_count(cell_array, y, x);
                printf("%d", alive_neighbor_count);
            } else {
                printf("%c", cell_array_get(cell_array, y, x) ? 'X' : '.');
            }
        }
        printf("\n");
    }
}

void erase_screen(void) {
    printf("\x1B[2J");
}

void enable_raw_mode() {
    // https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
    struct termios new_termios;

    tcgetattr(STDIN_FILENO, &new_termios);

    // Disable Echoing and Canonical mode
    new_termios.c_lflag &= (~ECHO & ~ICANON);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios);
}

void cursor_visible(bool visible) {
    if (visible) {
        printf("\x1B[?25h\n");
    } else {
        printf("\x1B[?25l\n");
    }
}

void cursor_move_home(void) { printf("\x1B[H"); }

void cursor_move(uint32_t x, uint32_t y) { printf("\x1B[%d;%dH", y, x); }

static bool running = true;

void *check_input(void *vargp) {
    char input = getchar();

    if (input == 'q' || input == 'Q') {
        running = false;
    }

    return NULL;
}

void step(Cell_Array_2d *grid) {
    Cell_Array_2d new_grid = cell_array_init(grid->width, grid->height);

    for (size_t y = 0; y < grid->height; y++) {
        for (size_t x = 0; x < grid->height; x++) {
            uint8_t alive_neighbor_count = cell_array_alive_neighbor_count(*grid, y, x);

            if (cell_array_get(*grid, y, x) == true) {
                // Alive Cell
                switch (alive_neighbor_count) {
                case 0:
                case 1:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8: {
                    // Die
                    cell_array_set(&new_grid, y, x, false);
                    break;
                }

                case 2:
                case 3: {
                    // Live
                    cell_array_set(&new_grid, y, x, true);
                    break;
                }
                }
            } else {
                // Dead Cell
                if (alive_neighbor_count == 3) {
                    // Resurrect
                    cell_array_set(&new_grid, y, x, true);
                }
            }
        }
    }

    cell_array_free(*grid);
    *grid = new_grid;
}

void render(Cell_Array_2d grid) {
    // Clear Screen
    cursor_move_home();
    erase_screen();

    // Print game
    printf("Press Q to exit.\n\n");
    cell_array_print(grid, false);
}

int32_t main(void) {
    Cell_Array_2d grid = cell_array_init(100, 100);

    // Init grid
    // TODO: make user specified
    cell_array_set(&grid, 15, 11, true);
    cell_array_set(&grid, 15, 12, true);
    cell_array_set(&grid, 16, 11, true);
    cell_array_set(&grid, 16, 12, true);

    cell_array_set(&grid, 13, 23, true);
    cell_array_set(&grid, 13, 24, true);
    cell_array_set(&grid, 14, 22, true);
    cell_array_set(&grid, 14, 26, true);
    cell_array_set(&grid, 15, 21, true);
    cell_array_set(&grid, 15, 27, true);
    cell_array_set(&grid, 16, 21, true);
    cell_array_set(&grid, 16, 25, true);
    cell_array_set(&grid, 16, 27, true);
    cell_array_set(&grid, 16, 28, true);
    cell_array_set(&grid, 17, 27, true);
    cell_array_set(&grid, 17, 21, true);
    cell_array_set(&grid, 18, 22, true);
    cell_array_set(&grid, 18, 26, true);
    cell_array_set(&grid, 19, 23, true);
    cell_array_set(&grid, 19, 24, true);

    cell_array_set(&grid, 11, 35, true);
    cell_array_set(&grid, 12, 33, true);
    cell_array_set(&grid, 12, 35, true);
    cell_array_set(&grid, 13, 31, true);
    cell_array_set(&grid, 13, 32, true);
    cell_array_set(&grid, 14, 31, true);
    cell_array_set(&grid, 14, 32, true);
    cell_array_set(&grid, 15, 31, true);
    cell_array_set(&grid, 15, 32, true);
    cell_array_set(&grid, 16, 33, true);
    cell_array_set(&grid, 16, 35, true);
    cell_array_set(&grid, 17, 35, true);

    cell_array_set(&grid, 13, 45, true);
    cell_array_set(&grid, 13, 46, true);
    cell_array_set(&grid, 14, 45, true);
    cell_array_set(&grid, 14, 46, true);

    // TODO: user input
    bool step_manually = false;

    // Init terminal and Quit input
    cursor_visible(false);
    enable_raw_mode();
    pthread_t input_thread_id;
    if (!step_manually) {
        pthread_create(&input_thread_id, NULL, check_input, NULL);
    }

    {
        #define US_PER_FRAME 400000 // ~60 FPS
        /* #define US_PER_FRAME 1 // MAX SPEED */
        struct timeval time_delta, time_start, time_end;
        double accumulator = 0;

        // Timekeeping
        while (running) {
            gettimeofday(&time_start, NULL);
            time_delta.tv_usec = time_end.tv_usec - time_start.tv_usec;
            // C is just too fast
            accumulator += time_delta.tv_usec == 0 ? 1 : time_delta.tv_usec;

            // :main loop
            if (step_manually) {
                char input = ' ';
                while (input == ' ') {
                    render(grid);

                    input = getchar();
                    if (input == 'q') {
                        running = false;
                        break;
                    }

                    step(&grid);
                }
            } else {
                while (accumulator >= US_PER_FRAME) {
                    accumulator -= US_PER_FRAME;
                    step(&grid);
                    render(grid);
                }
            }

            gettimeofday(&time_end, NULL);
        }
    }

    // Uninit terminal and Quit input
    if (!step_manually) {
        pthread_join(input_thread_id, NULL);
    }
    cursor_visible(true);

    // Free Grid memory
    cell_array_free(grid);

    return EX_OK;
}
