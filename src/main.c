#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <sys/param.h>
#include <sys/time.h>

typedef enum {
    EX_OK                   =   0,
    EX_ARR_OUT_OF_RANGE     = 100,
    EX_MEMORY_ALLOCATION    = 101,
    EX_ARGUMENT_PARSE_ERROR = 102,
} Exit_Codes;

#define ARR_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

#define PRINT_ERR_LOC(fmt, ...) fprintf(stderr, "[ERROR] %s:%d: " fmt, __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)
#define PRINT_ERR(fmt, ...) fprintf(stderr, "[ERROR] " fmt __VA_OPT__(,) __VA_ARGS__)

#define CELL_ARRAY_BOUNDS_CHECK(grid, y, x)                                          \
    if (y > grid.rows) {                                                             \
        PRINT_ERR_LOC("Cell Array index out of range! Y Coordinate is too big.\n");  \
        exit(EX_ARR_OUT_OF_RANGE);                                                   \
    }                                                                                \
    if (x > grid.cols) {                                                             \
        PRINT_ERR_LOC("Cell Array index out of range! X Coordinate is too big.\n");  \
        exit(EX_ARR_OUT_OF_RANGE);                                                   \
    }

#define CELL_ARRAY_PTR_BOUNDS_CHECK(grid, y, x)                                      \
    if (y > grid->rows) {                                                            \
        PRINT_ERR_LOC("Cell Array index out of range! Y Coordinate is too big.\n");  \
        exit(EX_ARR_OUT_OF_RANGE);                                                   \
    }                                                                                \
    if (x > grid->cols) {                                                            \
        PRINT_ERR_LOC("Cell Array index out of range! X Coordinate is too big.\n");  \
        exit(EX_ARR_OUT_OF_RANGE);                                                   \
    }

/**
*  A 2d array of cells.
*/
typedef struct {
    bool **cells;
    size_t cols;
    size_t rows;
} Cell_Array_2d;

bool cell_array_get(Cell_Array_2d cell_array, size_t y, size_t x) {
    CELL_ARRAY_BOUNDS_CHECK(cell_array, y, x);

    return cell_array.cells[y][x];
}

void cell_array_set(Cell_Array_2d *cell_array, size_t y, size_t x, bool value) {
    CELL_ARRAY_PTR_BOUNDS_CHECK(cell_array, y, x);

    cell_array->cells[y][x] = value;
}

Cell_Array_2d cell_array_init(size_t rows, size_t cols) {
    Cell_Array_2d cell_array = {
        .cells = malloc(sizeof(bool*) * rows),
        .rows = rows,
        .cols = cols,
    };
    if (cell_array.cells == NULL) {
        PRINT_ERR_LOC("Failed allocating memory for a Cell Array!");
        exit(EX_MEMORY_ALLOCATION);
    }
    for (size_t y = 0; y < rows; y++) {
        cell_array.cells[y] = malloc(sizeof(cell_array.cells[0]) * cols);

        if (cell_array.cells[y] == NULL) {
            // Free previously allocated rows.
            for (size_t y_old = 0; y_old < y; y_old++) {
                free(cell_array.cells[y_old]);
            }
            free(cell_array.cells);

            PRINT_ERR_LOC("Failed allocating memory for a Cell Array!");
            exit(EX_MEMORY_ALLOCATION);
        }
    }

    for (size_t y = 0; y < rows; y++) {
        for (size_t x = 0; x < cols; x++) {
            cell_array.cells[y][x] = false;
        }
    }

    return cell_array;
}

void cell_array_free(Cell_Array_2d cell_array) {
    for (size_t idx = 0; idx < cell_array.rows; idx++) {
        free(cell_array.cells[idx]);
    }
    free(cell_array.cells);
    cell_array.cols = 0;
    cell_array.rows = 0;
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

        if (y_idx_unsigned >= cell_array.rows || x_idx_unsigned >= cell_array.cols) {
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
    for (size_t y = 0; y < cell_array.rows; y++) {
        for (size_t x = 0; x < cell_array.cols; x++) {
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void *check_input_terminal(void *vargp) {
#pragma GCC diagnostic pop
    char input = getchar();

    if (input == 'q' || input == 'Q') {
        running = false;
    }

    return NULL;
}

void step(Cell_Array_2d *grid) {
    Cell_Array_2d new_grid = cell_array_init(grid->rows, grid->cols);

    for (size_t y = 0; y < grid->rows; y++) {
        for (size_t x = 0; x < grid->cols; x++) {
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

void render_terminal(Cell_Array_2d grid) {
    // Clear Screen
    cursor_move_home();
    erase_screen();

    // Print game
    printf("Press Space to step through. Press Q to exit.\n\n");
    cell_array_print(grid, false);
}

void run_terminal(Cell_Array_2d *grid, bool step_manually) {
    // Init terminal and Quit input
    cursor_visible(false);
    enable_raw_mode();
    pthread_t input_thread_id;
    if (!step_manually) {
        pthread_create(&input_thread_id, NULL, check_input_terminal, NULL);
    }

    {
        #define US_PER_FRAME 400000 // ~60 FPS
        /* #define US_PER_FRAME 1 // MAX SPEED */
        struct timeval time_delta, time_start, time_end;
        double accumulator = 0;
        gettimeofday(&time_end, NULL);

        // Timekeeping
        while (running) {
            gettimeofday(&time_start, NULL);
            time_delta.tv_usec = time_end.tv_usec - time_start.tv_usec;
            // C is just too fast
            accumulator += time_delta.tv_usec == 0 ? 1 : time_delta.tv_usec;

            if (step_manually) {
                char input = ' ';
                while (input != 'q') {
                    render_terminal(*grid);

                    input = getchar();
                    if (input == 'q') {
                        running = false;
                        break;
                    }

                    step(grid);
                }
            } else {
                while (accumulator >= US_PER_FRAME) {
                    accumulator -= US_PER_FRAME;
                    step(grid);

                    render_terminal(*grid);
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
}

void run_raylib(Cell_Array_2d *grid, bool step_manually) {
    #include "raylib.h"

    const size_t cell_padding = 1;
    const size_t grid_padding = 10;
    double window_width = 600;
    double window_height = 600;
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    InitWindow(window_width, window_height, "DEBUG");

    // Magic timekeeping variables for a fixed timestep from the raylib examples:
    // https://www.raylib.com/examples/core/loader.html?name=core_custom_frame_control
    double previous_time_s = GetTime();
    const size_t target_ups = 32;

    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_Q)) {
            break;
        }

        if (IsWindowResized()) {
            window_width = GetScreenWidth();
            window_height = GetScreenHeight();
        }
        window_width = MIN(window_width, window_height);
        window_height = MIN(window_width, window_height);

        double center_y = window_height / 2;
        double center_x = window_width / 2;
        double grid_area_width = window_width - grid_padding * 2;
        double grid_area_height = window_height - grid_padding * 2;
        double cell_width = (grid_area_width / grid->cols) - cell_padding;
        double cell_height = (grid_area_height / grid->rows) - cell_padding;

        BeginDrawing();
        {
            ClearBackground(RAYWHITE);

            // Draw Grid
            for (size_t y = 0; y <= grid->rows; y++) {
                DrawLineEx(
                    (Vector2) {
                        .x = grid_padding,
                        .y = grid_padding + cell_height * y + cell_padding * y,
                    },
                    (Vector2) {
                        .x = window_width - grid_padding,
                        .y = grid_padding + cell_height * y + cell_padding * y,
                    },
                    1,
                    GRAY
                );
            }
            for (size_t x = 0; x <= grid->cols; x++) {
                DrawLineEx(
                    (Vector2) {
                        .x = grid_padding + cell_width * x + cell_padding * x,
                        .y = grid_padding,
                    },
                    (Vector2) {
                        .x = grid_padding + cell_width * x + cell_padding * x,
                        .y = window_height - grid_padding,
                    },
                    1,
                    GRAY
                );
            }

            for (size_t y = 0; y < grid->rows; y++) {
                for (size_t x = 0; x < grid->cols; x++) {
                    if (cell_array_get(*grid, y, x) == false) {
                        continue;
                    }

                    DrawRing(
                        (Vector2) {
                            .x = grid_padding + cell_width / 2 + cell_width * x + cell_padding * x,
                            .y = grid_padding + cell_height / 2 + cell_height * y + cell_padding * y,
                        },
                        0,
                        MIN(cell_width, cell_height) / 3,
                        0,
                        360,
                        0,
                        GREEN
                    );
                }
            }
        }
        EndDrawing();

        if (step_manually) {
            if (IsKeyDown(KEY_SPACE)) {
                step(grid);
                WaitTime(0.07);
            }
        } else {
            // Magic timekeeping for a fixed timestep from the raylib examples:
            // https://www.raylib.com/examples/core/loader.html?name=core_custom_frame_control
            double current_time_s = GetTime();
            double update_draw_time_s = current_time_s - previous_time_s;
            if (target_ups > 0) {
                double wait_time_s = (1.0f/(float)target_ups) - update_draw_time_s;
                if (wait_time_s > 0.0) {
                    WaitTime((float)wait_time_s);
                    current_time_s = GetTime();

                    step(grid);
                }
            }
            previous_time_s = current_time_s;
        }
    }

    CloseWindow();
}

typedef struct {
    size_t grid_rows;
    size_t grid_cols;

    bool step_manually;
    bool raylib;
} Config;

Config parse_arguments(int argc, char *argv[]) {
    Config config = {
        .grid_rows = 69,
        .grid_cols = 69,
    };

    #define PRINT_USAGE()                                                                    \
        printf(                                                                              \
            "Simulate Conway's Game of Life either in the terminal or a graphical window.\n" \
            "\n"                                                                             \
            "Usage: conway [options]\n"                                                      \
            "\n"                                                                             \
            "Options:\n"                                                                     \
            "    -h, --help\n"                                                               \
            "        Print this message.\n"                                                  \
            "\n"                                                                             \
            "    --grid-rows <positive number>\n"                                            \
            "    --grid-cols <positive number>\n"                                            \
            "    --step-manually\n"                                                          \
            "        Step manually by pressing SPACE.\n"                                     \
            "    --graphical, --raylib\n"                                                    \
            "        Display the game using a graphical interface (with Raylib btw).\n"      \
        )

    for (size_t idx = 0; idx < argc; idx++) {
        char *arg = argv[idx];

        if (arg[0] == '-') {
            char *name = &arg[1];

            if (strcmp(name, "h") == 0) {
                PRINT_USAGE();
                break;
            }

            if (arg[1] == '-') {
                name = &arg[2];

                if (strcmp(name, "help") == 0) {
                    PRINT_USAGE();
                    break;
                }

                if (idx >= argc - 1) {
                    PRINT_ERR("Argument '%s' needs to have a value!\n", name);
                }
                char *value = argv[idx + 1];

                if (strcmp(name, "grid-rows") == 0) {
                    size_t rows = atoi(value);
                    if (rows == 0) {
                        PRINT_ERR("Grid rows should be bigger than 0.\n");
                        exit(EX_ARGUMENT_PARSE_ERROR);
                    }

                    config.grid_rows = atoi(value);
                } else
                if (strcmp(name, "grid-cols") == 0) {
                    size_t cols = atoi(value);
                    if (cols == 0) {
                        PRINT_ERR("Grid columns should be bigger than 0.\n");
                        exit(EX_ARGUMENT_PARSE_ERROR);
                    }

                    config.grid_cols = atoi(value);
                } else
                if (strcmp(name, "step-manually") == 0) {
                    config.step_manually = true;
                } else
                if (strcmp(name, "graphical") == 0) {
                    config.raylib = true;
                } else
                if (strcmp(name, "raylib") == 0) {
                    config.raylib = true;
                }
            }
        }
    }

    return config;
}

int32_t main(int argc, char *argv[]) {
    Config config = parse_arguments(argc, argv);
    Cell_Array_2d grid = cell_array_init(config.grid_rows, config.grid_cols);

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

    if (config.raylib) {
        run_raylib(&grid, config.step_manually);
    } else {
        run_terminal(&grid, config.step_manually);
    }

    // Free Grid memory
    cell_array_free(grid);

    return EX_OK;
}
