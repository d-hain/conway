#define _DEFAULT_SOURCE // Needed for getline() function
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
#include "raylib.h"

typedef enum {
    EX_OK                   =   0,
    EX_ARR_OUT_OF_RANGE     = 100,
    EX_MEMORY_ALLOCATION    = 101,
    EX_ARGUMENT_PARSE_ERROR = 102,
    EX_INPUT_READ_ERROR     = 104,
    EX_SHOW_USAGE           = 105,
} Exit_Codes;

#define ARR_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

#define PRINT_ERR_LOC(fmt, ...) fprintf(stderr, "[ERROR] %s:%d: " fmt, __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)
#define PRINT_ERR(fmt, ...) fprintf(stderr, "[ERROR] " fmt __VA_OPT__(,) __VA_ARGS__)

#define CELL_ARRAY_BOUNDS_CHECK(grid, row, col)                                      \
    if (row > grid.rows) {                                                           \
        PRINT_ERR_LOC("Cell Array index out of range! Y Coordinate is too big.\n");  \
        cell_array_free(grid);                                                       \
        exit(EX_ARR_OUT_OF_RANGE);                                                   \
    }                                                                                \
    if (col > grid.cols) {                                                           \
        PRINT_ERR_LOC("Cell Array index out of range! X Coordinate is too big.\n");  \
        cell_array_free(grid);                                                       \
        exit(EX_ARR_OUT_OF_RANGE);                                                   \
    }

#define CELL_ARRAY_PTR_BOUNDS_CHECK(grid, row, col)                                  \
    if (row > grid->rows) {                                                          \
        PRINT_ERR_LOC("Cell Array index out of range! Y Coordinate is too big.\n");  \
        cell_array_free_ptr(grid);                                                   \
        exit(EX_ARR_OUT_OF_RANGE);                                                   \
    }                                                                                \
    if (col > grid->cols) {                                                          \
        PRINT_ERR_LOC("Cell Array index out of range! X Coordinate is too big.\n");  \
        cell_array_free_ptr(grid);                                                   \
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

Cell_Array_2d cell_array_init(const size_t rows, const size_t cols) {
    Cell_Array_2d cell_array = {
        .cells = malloc(sizeof(bool*) * rows),
        .rows = rows,
        .cols = cols,
    };
    if (cell_array.cells == NULL) {
        PRINT_ERR_LOC("Failed allocating memory for a Cell Array!");
        exit(EX_MEMORY_ALLOCATION);
    }
    for (size_t row = 0; row < rows; row++) {
        cell_array.cells[row] = malloc(sizeof(cell_array.cells[0]) * cols);

        if (cell_array.cells[row] == NULL) {
            // Free previously allocated rows.
            for (size_t y_old = 0; y_old < row; y_old++) {
                free(cell_array.cells[y_old]);
            }
            free(cell_array.cells);

            PRINT_ERR_LOC("Failed allocating memory for a Cell Array!");
            exit(EX_MEMORY_ALLOCATION);
        }
    }

    for (size_t row = 0; row < rows; row++) {
        for (size_t col = 0; col < cols; col++) {
            cell_array.cells[row][col] = false;
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

void cell_array_free_ptr(Cell_Array_2d *cell_array) {
    for (size_t idx = 0; idx < cell_array->rows; idx++) {
        free(cell_array->cells[idx]);
    }
    free(cell_array->cells);
    cell_array->cols = 0;
    cell_array->rows = 0;
    cell_array = NULL;
}

bool cell_array_get(const Cell_Array_2d cell_array, const size_t row, const size_t col) {
    CELL_ARRAY_BOUNDS_CHECK(cell_array, row, col);

    return cell_array.cells[row][col];
}

void cell_array_set(Cell_Array_2d *cell_array, const size_t row, const size_t col, const bool value) {
    CELL_ARRAY_PTR_BOUNDS_CHECK(cell_array, row, col);

    cell_array->cells[row][col] = value;
}

uint8_t cell_array_alive_neighbor_count(const Cell_Array_2d cell_array, const size_t row, const size_t col) {
    CELL_ARRAY_BOUNDS_CHECK(cell_array, row, col);

    const int32_t indices[8][2] = {
        {row, col - 1}, // Left
        {row, col + 1}, // Right
        {row - 1, col}, // Top
        {row - 1, col - 1}, // Left-Top
        {row - 1, col + 1}, // Right-Top
        {row + 1, col}, // Bottom
        {row + 1, col - 1}, // Left-Bottom
        {row + 1, col + 1}, // Right-Bottom
    };

    uint8_t alive_neighbor_count = 0;
    for (size_t i = 0; i < ARR_LEN(indices); i++) {
        // Validate indices
        const int32_t row_idx = indices[i][0];
        const int32_t col_idx = indices[i][1];

        if (row_idx < 0 || col_idx < 0) {
            continue;
        }
        const size_t row_idx_unsigned = row_idx;
        const size_t col_idx_unsigned = col_idx;

        if (row_idx_unsigned >= cell_array.rows || col_idx_unsigned >= cell_array.cols) {
            continue;
        }

        // Index is valid -> check if alive
        if (cell_array_get(cell_array, row_idx_unsigned, col_idx_unsigned) == true) {
            alive_neighbor_count++;
        }
    }

    return alive_neighbor_count;
}

typedef enum {
    COLOR_SCHEME_DEFAULT = 0,
    COLOR_SCHEME_HACKER  = 1,
} Color_Scheme;
#define COLOR_SCHEME_COUNT (COLOR_SCHEME_HACKER - COLOR_SCHEME_DEFAULT) + 1

const char *color_scheme_to_string(const Color_Scheme color_scheme) {
    switch (color_scheme) {
        case COLOR_SCHEME_DEFAULT: return "default";
        case COLOR_SCHEME_HACKER:  return "hacker";
    }
    return "";
}

void cell_array_print(const Cell_Array_2d cell_array, const Color_Scheme color_scheme) {
    for (size_t row = 0; row < cell_array.rows; row++) {
        for (size_t col = 0; col < cell_array.cols; col++) {
            char empty_cell = '.';
            switch (color_scheme) {
                case COLOR_SCHEME_DEFAULT: empty_cell = '.'; break;
                case COLOR_SCHEME_HACKER:  empty_cell = ' '; break;
            }

            printf("%c", cell_array_get(cell_array, row, col) ? 'X' : empty_cell);
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

void cursor_visible(const bool visible) {
    if (visible) {
        printf("\x1B[?25h\n");
    } else {
        printf("\x1B[?25l\n");
    }
}

void cursor_move_home(void) { printf("\x1B[H"); }

void cursor_move(const uint32_t x, const uint32_t y) { printf("\x1B[%d;%dH", y, x); }

void change_fg_color(const uint8_t color) { printf("\x1B[38;5;%dm", color); }

void change_bg_color(const uint8_t color) { printf("\x1B[48;5;%dm", color); }

void clear_color(void) { printf("\x1B[0m"); }

static bool running = true;

void ctrlc_handler(int _signum) {
    (void)_signum; // Unused parameter
    running = false;
}

// Catches the CTRL+C signal and calls the `ctrlc_handler` function.
// NOTE: $ man 2 sigaction
void setup_ctrlc_handler(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = ctrlc_handler;
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void *check_input_terminal(void *vargp) {
#pragma GCC diagnostic pop
    const char input = getchar();

    if (input == 'q' || input == 'Q') {
        running = false;
    }

    return NULL;
}

void step(Cell_Array_2d *grid) {
    Cell_Array_2d new_grid = cell_array_init(grid->rows, grid->cols);

    for (size_t row = 0; row < grid->rows; row++) {
        for (size_t col = 0; col < grid->cols; col++) {
            const uint8_t alive_neighbor_count = cell_array_alive_neighbor_count(*grid, row, col);

            if (cell_array_get(*grid, row, col) == true) {
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
                    cell_array_set(&new_grid, row, col, false);
                    break;
                }

                case 2:
                case 3: {
                    // Live
                    cell_array_set(&new_grid, row, col, true);
                    break;
                }
                }
            } else {
                // Dead Cell
                if (alive_neighbor_count == 3) {
                    // Resurrect
                    cell_array_set(&new_grid, row, col, true);
                }
            }
        }
    }

    cell_array_free(*grid);
    *grid = new_grid;
}

void render_terminal(const Cell_Array_2d grid, const Color_Scheme color_scheme) {
    // Clear Screen
    cursor_move_home();
    erase_screen();

    // Print game
    printf("Press Space to step through. Press Q to exit.\n\n");
    switch (color_scheme) {
        case COLOR_SCHEME_DEFAULT: break;
        case COLOR_SCHEME_HACKER: change_bg_color(0); change_fg_color(46); break;
    }
    cell_array_print(grid, color_scheme);
    clear_color();
}

bool is_digit(const char input) {
    switch (input) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return true;
    }

    return false;
}

void set_starting_input(Cell_Array_2d *grid, const char *input, const size_t input_len) {
    if (input_len == 0) return;

    // Parse user input
    #define PARSE_AND_SET_NUMBERS()                                 \
        if (numbers[0].is_some && numbers[1].is_some) {             \
            size_t positions[2] = {0};                              \
            positions[0] = atoi(numbers[0].digits);                 \
            positions[1] = atoi(numbers[1].digits);                 \
            cell_array_set(grid, positions[0], positions[1], true); \
        }

    typedef struct {
        // Up to 99_999 must be enough.
        char digits[5];
        bool is_some;
    } Number_Or_None;

    Number_Or_None numbers[2] = {}; // [0] => row, [1] => col
    size_t number_idx = 0;
    size_t digit_idx = 0;
    for (size_t idx = 0; idx < input_len; idx++) {
        char c = input[idx];

        if (c == ' ') {
            if (number_idx == 1) {
                PARSE_AND_SET_NUMBERS();
            }

            number_idx = 0;
            digit_idx = 0;
            for (size_t i = 0; i < 2; i++) {
                numbers[i].is_some = false;
                for (size_t j = 0; j < 5; j++) {
                    numbers[i].digits[j] = 0;
                }
            }
            continue;
        }

        if (is_digit(c)) {
            numbers[number_idx].is_some = true;
            numbers[number_idx].digits[digit_idx] = c;
            digit_idx++;
        } else
        if (c == ',') {
            number_idx = 1;
            digit_idx = 0;
        }
    }
    PARSE_AND_SET_NUMBERS();
}

void terminal_get_starting_input(Cell_Array_2d *grid, const Color_Scheme color_scheme) {
    render_terminal(*grid, color_scheme);
    printf(
        "Give some starting input.\n"
        "The top left is 0,0 and the format is row,col.\n"
        "Example: 2,2 2,3 3,2 3,3\n"
        "\n"
        "> "
    );

    char *line = NULL;
    size_t n = 0;
    const ssize_t line_length = getline(&line, &n, stdin);
    if (line_length == -1) {
        PRINT_ERR("Failed reading starting input!\n");
        free(line);
        exit(EX_INPUT_READ_ERROR);
    }
    // Remove newline
    line[line_length - 1] = '\0';

    set_starting_input(grid, line, line_length);

    free(line);
}

void run_terminal(Cell_Array_2d *grid, const bool step_manually, const Color_Scheme color_scheme) {
    setup_ctrlc_handler();
    terminal_get_starting_input(grid, color_scheme);

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
                while (input == ' ') {
                    render_terminal(*grid, color_scheme);

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

                    render_terminal(*grid, color_scheme);
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

/**
*   # Returns
*
*   The grid area width and height.
*/
Vector2 raylib_draw_grid(
    const Cell_Array_2d grid,
    const size_t grid_padding_top,
    const size_t grid_padding_right,
    const size_t grid_padding_left,
    const size_t grid_padding_bottom,
    const size_t cell_padding,
    const double window_width,
    const double window_height,
    const Color_Scheme color_scheme,
    const bool draw_grid
) {
    const double grid_area_width = window_width - grid_padding_left - grid_padding_right;
    const double grid_area_height = window_height - grid_padding_top - grid_padding_bottom;
    const double cell_width = (grid_area_width / grid.cols) - cell_padding;
    const double cell_height = (grid_area_height / grid.rows) - cell_padding;

    // Draw Grid
    // Horizontal lines
    if (color_scheme == COLOR_SCHEME_DEFAULT || draw_grid) {
        for (size_t row = 0; row <= grid.rows; row++) {
            DrawLineEx(
                (Vector2) {
                    .x = grid_padding_left,
                    .y = grid_padding_top + cell_height * row + cell_padding * row,
                },
                (Vector2) {
                    .x = window_width - grid_padding_right,
                    .y = grid_padding_top + cell_height * row + cell_padding * row,
                },
                1,
                GRAY
            );
        }
        // Vertical lines
        for (size_t col = 0; col <= grid.cols; col++) {
            DrawLineEx(
                (Vector2) {
                    .x = grid_padding_left + cell_width * col + cell_padding * col,
                    .y = grid_padding_top,
                },
                (Vector2) {
                    .x = grid_padding_left + cell_width * col + cell_padding * col,
                    .y = window_height - grid_padding_bottom,
                },
                1,
                GRAY
            );
        }
    }

    // Draw Alive Cells
    for (size_t row = 0; row < grid.rows; row++) {
        for (size_t col = 0; col < grid.cols; col++) {
            if (cell_array_get(grid, row, col) == false) {
                continue;
            }

            switch (color_scheme) {
            case COLOR_SCHEME_DEFAULT: {
                DrawRing(
                    (Vector2) {
                        .x = grid_padding_left + cell_width / 2 + cell_width * col + cell_padding * col,
                        .y = grid_padding_top + cell_height / 2 + cell_height * row + cell_padding * row,
                    },
                    0,
                    MIN(cell_width, cell_height) / 3,
                    0,
                    360,
                    0,
                    GREEN
                );
                break;
            }

            case COLOR_SCHEME_HACKER: {
                DrawRectangleV(
                    (Vector2) {
                        .x = grid_padding_left + cell_width * col + cell_padding * col,
                        .y = grid_padding_top + cell_height * row + cell_padding * row,
                    },
                    (Vector2) {
                        .x = cell_width,
                        .y = cell_height,
                    },
                    GREEN
                );
            break;
            }
            }
        }
    }

    return (Vector2) { .x = grid_area_width, .y = grid_area_height };
}

void run_raylib(Cell_Array_2d *grid, const bool step_manually, const bool show_fps, const Color_Scheme color_scheme) {
    typedef enum {
        STATE_PLACING,
        STATE_SIMULATING,
    } State;
    State state = STATE_PLACING;

    const size_t cell_padding = 1;
    const size_t grid_padding = 10;

    // HACK: Is this how you do that in raylib?! Well what works works ig...
    InitWindow(1, 1, "Temporary window to get the monitor");
    const int current_monitor = GetCurrentMonitor();
    const int smallest_monitor_dimension = MIN(GetMonitorWidth(current_monitor), GetMonitorHeight(current_monitor));
    double window_width = smallest_monitor_dimension / 1.8;
    double window_height = smallest_monitor_dimension / 1.8;
    CloseWindow();

    SetWindowState(FLAG_WINDOW_RESIZABLE);
    InitWindow(window_width, window_height, "Conway");

    // Magic timekeeping variables for a fixed timestep from the raylib examples:
    // https://www.raylib.com/examples/core/loader.html?name=core_custom_frame_control
    double previous_time_s = GetTime();
    const size_t target_ups = 32;

    const size_t font_size = 24;
    const Vector2 text_pos = {
        .x = 10,
        .y = 10,
    };
    Color text_color = RAYWHITE;
    switch (color_scheme) {
        case COLOR_SCHEME_DEFAULT: text_color = BLACK; break;
        case COLOR_SCHEME_HACKER: text_color = RAYWHITE; break;
    }

    const size_t start_button_font_size = font_size - 4;
    const char *start_button_text = "START";
    const Vector2 start_button_text_padding = { 10, 5 };
    const size_t start_button_text_width = MeasureText(start_button_text, start_button_font_size);
    const float start_button_width = start_button_text_width + start_button_text_padding.x * 2;

    #define DRAW_BACKGROUND()                                        \
        switch (color_scheme) {                                      \
        case COLOR_SCHEME_DEFAULT: ClearBackground(RAYWHITE); break; \
        case COLOR_SCHEME_HACKER: ClearBackground(BLACK); break;     \
        }

    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_Q)) {
            break;
        }

        if (IsWindowResized()) {
            window_width = GetScreenWidth();
            window_height = GetScreenHeight();
        }

        switch (state) {
        case STATE_PLACING: {
            const Vector2 mouse_pos = GetMousePosition();
            const Rectangle start_button = {
                .x = window_width - 10 - start_button_width,
                .y = 5,
                .width = start_button_width,
                .height = start_button_font_size + start_button_text_padding.y * 2,
            };

            BeginDrawing();
            {
                DRAW_BACKGROUND();

                const size_t grid_padding_top = grid_padding + font_size + text_pos.y;
                const Vector2 grid_area_size = raylib_draw_grid(
                    *grid,
                    grid_padding_top, grid_padding, grid_padding, grid_padding,
                    cell_padding,
                    window_width,
                    window_height,
                    color_scheme,
                    true
                );

                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    // Place the starting cells
                    const size_t mouse_row = (mouse_pos.y - grid_padding_top) / (grid_area_size.y / grid->rows);
                    const size_t mouse_col = (mouse_pos.x - grid_padding) / (grid_area_size.x / grid->cols);
                    if (mouse_row < grid->rows && mouse_col < grid->cols) {
                        cell_array_set(grid, mouse_row, mouse_col, true);
                    }

                    // Press Start Button
                    if (CheckCollisionPointRec(mouse_pos, start_button)) {
                        state = STATE_SIMULATING;
                    }
                }

                DrawText("Set the starting input using left click.", text_pos.x, text_pos.y, font_size, text_color);

                // Draw Start Button
                DrawRectangleRec(start_button, GRAY);
                DrawText(
                    start_button_text,
                    start_button.x + start_button_text_padding.x,
                    start_button.y + start_button_text_padding.y,
                    start_button_font_size,
                    ORANGE
                );
            }
            EndDrawing();
            break;
        }

        case STATE_SIMULATING: {
            BeginDrawing();
            {
                DRAW_BACKGROUND();

                raylib_draw_grid(
                    *grid,
                    grid_padding, grid_padding, grid_padding, grid_padding,
                    cell_padding,
                    window_width,
                    window_height,
                    color_scheme,
                    false
                );

                if (show_fps) {
                    DrawFPS(0, 0);
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
                const double update_draw_time_s = current_time_s - previous_time_s;
                if (target_ups > 0) {
                    const double wait_time_s = (1.0f/(float)target_ups) - update_draw_time_s;
                    if (wait_time_s > 0.0) {
                        WaitTime((float)wait_time_s);
                        current_time_s = GetTime();

                        step(grid);
                    }
                }
                previous_time_s = current_time_s;
            }

            break;
        }
        }
    }

    CloseWindow();
}

typedef struct {
    size_t grid_rows;
    size_t grid_cols;

    bool step_manually;
    bool raylib;
    bool show_fps;
    bool glider_gun;
    Color_Scheme color_scheme;

    char *starting_input;
} Config;

Config parse_arguments(const unsigned int argc, char *argv[]) {
    Config config = {
        .grid_rows = 69,
        .grid_cols = 69,
        .step_manually = false,
        .raylib = false,
        .show_fps = false,
        .glider_gun = false,
        .starting_input = "",
        .color_scheme = COLOR_SCHEME_DEFAULT,
    };

    #define PRINT_USAGE()                                                                                           \
        printf(                                                                                                     \
            "Simulate Conway's Game of Life either in the terminal or a graphical window.\n"                        \
            "\n"                                                                                                    \
            "Usage: conway [options]\n"                                                                             \
            "\n"                                                                                                    \
            "Options:\n"                                                                                            \
            "    -h, --help\n"                                                                                      \
            "        Print this message.\n"                                                                         \
            "\n"                                                                                                    \
            "    --grid-rows <positive number>\n"                                                                   \
            "    --grid-cols <positive number>\n"                                                                   \
            "\n"                                                                                                    \
            "    --step-manually\n"                                                                                 \
            "        Step manually by pressing SPACE.\n"                                                            \
            "\n"                                                                                                    \
            "    --graphical, --raylib\n"                                                                           \
            "        Display the game using a graphical interface (with Raylib btw).\n"                             \
            "\n"                                                                                                    \
            "    --show-fps\n"                                                                                      \
            "        Show the FPS when rendering using raylib.\n"                                                   \
            "\n"                                                                                                    \
            "    --glider-gun\n"                                                                                    \
            "        Start the game with Gosper's glider gun in the top left.\n"                                    \
            "\n"                                                                                                    \
            "    --starting-input <input>\n"                                                                        \
            "        Specify the starting input in a space and comma separated string like this:"                   \
            "           --starting-input \"<row>,<col> <row>,<col> ...\"\n"                                         \
            "\n"                                                                                                    \
            "    --color-scheme <color scheme>\n"                                                                   \
            "        Different funky colors.\n"                                                                     \
            "        Available color schemes:\n"                                                                    \
        );                                                                                                          \
        for (Color_Scheme color_scheme = COLOR_SCHEME_DEFAULT; color_scheme < COLOR_SCHEME_COUNT; color_scheme++) { \
            printf(                                                                                                 \
            "            %s\n", color_scheme_to_string(color_scheme)                                                \
            );                                                                                                      \
        }

    for (size_t idx = 0; idx < argc; idx++) {
        const char *arg = argv[idx];

        if (arg[0] == '-') {
            const char *name = &arg[1];

            if (strcmp(name, "h") == 0) {
                PRINT_USAGE();
                exit(EX_SHOW_USAGE);
            }

            if (arg[1] == '-') {
                name = &arg[2];

                // Options without a value
                if (strcmp(name, "help") == 0) {
                    PRINT_USAGE();
                    exit(EX_SHOW_USAGE);
                } else
                if (strcmp(name, "step-manually") == 0) {
                    config.step_manually = true;
                    continue;
                } else
                if (strcmp(name, "graphical") == 0) {
                    config.raylib = true;
                    continue;
                } else
                if (strcmp(name, "raylib") == 0) {
                    config.raylib = true;
                    continue;
                } else
                if (strcmp(name, "glider-gun") == 0) {
                    if (config.grid_rows < 12 || config.grid_cols < 38) {
                        PRINT_ERR(
                            "The glider gun only works with a minimun size of 12 rows by 38 columns! "
                            "(--glider-gun must be after --grid-rows and --grid-cols)\n"
                        );
                        exit(EX_ARGUMENT_PARSE_ERROR);
                    }
                    config.glider_gun = true;
                    continue;
                } else
                if (strcmp(name, "show-fps") == 0) {
                    if (config.raylib != true) {
                        PRINT_ERR("Showing FPS only works with raylib enabled! (--raylib must be before --show-fps)\n");
                        exit(EX_ARGUMENT_PARSE_ERROR);
                    }
                    config.show_fps = true;
                    continue;
                }

                if (idx >= argc - 1) {
                    PRINT_ERR("Argument '%s' needs to have a value!\n", name);
                    exit(EX_ARGUMENT_PARSE_ERROR);
                }
                char *value = argv[idx + 1];

                // Options with a value
                if (strcmp(name, "grid-rows") == 0) {
                    const size_t rows = atoi(value);
                    if (rows == 0) {
                        PRINT_ERR("Grid rows should be bigger than 0.\n");
                        exit(EX_ARGUMENT_PARSE_ERROR);
                    }

                    config.grid_rows = atoi(value);
                } else
                if (strcmp(name, "grid-cols") == 0) {
                    const size_t cols = atoi(value);
                    if (cols == 0) {
                        PRINT_ERR("Grid columns should be bigger than 0.\n");
                        exit(EX_ARGUMENT_PARSE_ERROR);
                    }

                    config.grid_cols = atoi(value);
                } else
                if (strcmp(name, "starting-input") == 0) {
                    config.starting_input = value;
                } else
                if (strcmp(name, "color-scheme") == 0) {
                    for (Color_Scheme color_scheme = COLOR_SCHEME_DEFAULT; color_scheme < COLOR_SCHEME_COUNT; color_scheme++) {
                        if (strcmp(value, color_scheme_to_string(color_scheme)) == 0) {
                            config.color_scheme = color_scheme;
                        }
                    }

                    if (config.color_scheme == COLOR_SCHEME_DEFAULT
                        && !(strcmp(value, color_scheme_to_string(COLOR_SCHEME_DEFAULT)) == 0)
                    ) {
                        PRINT_ERR("Invalid color scheme \"%s\"!\n", value);
                        PRINT_ERR("Valid color schemes are:\n");
                        for (Color_Scheme color_scheme = COLOR_SCHEME_DEFAULT; color_scheme < COLOR_SCHEME_COUNT; color_scheme++) {
                            PRINT_ERR("\t%s\n", color_scheme_to_string(color_scheme));
                        }
                        exit(EX_ARGUMENT_PARSE_ERROR);
                    }
                }
            }
        }
    }

    return config;
}

int32_t main(const int argc, char *argv[]) {
    const Config config = parse_arguments(argc, argv);
    Cell_Array_2d grid = cell_array_init(config.grid_rows, config.grid_cols);
    if (strcmp(config.starting_input, "") != 0) {
        set_starting_input(&grid, config.starting_input, strlen(config.starting_input));
    }

    // Init default grid pattern
    if (config.glider_gun) {
        cell_array_set(&grid, 5,  1, true);
        cell_array_set(&grid, 5,  2, true);
        cell_array_set(&grid, 6,  1, true);
        cell_array_set(&grid, 6,  2, true);

        cell_array_set(&grid, 3, 13, true);
        cell_array_set(&grid, 3, 14, true);
        cell_array_set(&grid, 4, 12, true);
        cell_array_set(&grid, 4, 16, true);
        cell_array_set(&grid, 5, 11, true);
        cell_array_set(&grid, 5, 17, true);
        cell_array_set(&grid, 6, 11, true);
        cell_array_set(&grid, 6, 15, true);
        cell_array_set(&grid, 6, 17, true);
        cell_array_set(&grid, 6, 18, true);
        cell_array_set(&grid, 7, 17, true);
        cell_array_set(&grid, 7, 11, true);
        cell_array_set(&grid, 8, 12, true);
        cell_array_set(&grid, 8, 16, true);
        cell_array_set(&grid, 9, 13, true);
        cell_array_set(&grid, 9, 14, true);

        cell_array_set(&grid, 1, 25, true);
        cell_array_set(&grid, 2, 23, true);
        cell_array_set(&grid, 2, 25, true);
        cell_array_set(&grid, 3, 21, true);
        cell_array_set(&grid, 3, 22, true);
        cell_array_set(&grid, 4, 21, true);
        cell_array_set(&grid, 4, 22, true);
        cell_array_set(&grid, 5, 21, true);
        cell_array_set(&grid, 5, 22, true);
        cell_array_set(&grid, 6, 23, true);
        cell_array_set(&grid, 6, 25, true);
        cell_array_set(&grid, 7, 25, true);

        cell_array_set(&grid, 3, 35, true);
        cell_array_set(&grid, 3, 36, true);
        cell_array_set(&grid, 4, 35, true);
        cell_array_set(&grid, 4, 36, true);
    }

    if (config.raylib) {
        run_raylib(&grid, config.step_manually, config.show_fps, config.color_scheme);
    } else {
        run_terminal(&grid, config.step_manually, config.color_scheme);
    }

    // Free Grid memory
    cell_array_free(grid);

    return EX_OK;
}
