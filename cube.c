#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include <raylib.h>
#include <ncurses.h>
#include <raymath.h>
#include <time.h>


#define WIDTH 1920
#define HEIGHT 1080
#define BACKGROUND_COLOR (Color) { 51, 51, 51, 255 }

#define RECT_SIZE 1


static Vector3 vector3_invert_y(Vector3 v) {
    v.y = 1.0f - v.y;
    return v;
}






typedef struct {
    Color **grid;
    size_t width, height;
} Canvas;

static Canvas canvas_new(size_t width, size_t height) {
    Canvas canvas = { .grid = NULL, .width = width, .height = height };

    canvas.grid = malloc(canvas.height * sizeof(Color*));

    for (size_t y=0; y < canvas.height; ++y) {
        canvas.grid[y] = malloc(canvas.width * sizeof(Color));
    }

    return canvas;
}

static void canvas_destroy(Canvas *canvas) {
    for (size_t i=0; i < canvas->height; ++i)
        free(canvas->grid[i]);
    free(canvas->grid);
    canvas->grid = NULL;
}

static void canvas_fill(Canvas *canvas, Color color) {
    for (size_t y=0; y < canvas->height; ++y)
        for (size_t x=0; x < canvas->width; ++x)
            canvas->grid[y][x] = color;
}

static void canvas_draw_line(Canvas *grid, Vector3 start, Vector3 end, Color color) {
    end.x += 1; // cannot draw perfect straight line, as end.x - start-x == 0
    float slope = (end.y - start.y) / (end.x - start.x);
    float step = 0.01f;
    for (float x=start.x; x < end.x; x+=step) {
        float y = slope * (x - start.x) + start.y;
        if (y > grid->height)
            continue;
        grid->grid[(size_t)y][(size_t)x] = color;
    }
}

static void canvas_render(const Canvas *canvas) {
    for (size_t y=0; y < canvas->height; ++y) {
        for (size_t x=0; x < canvas->width; ++x) {
            Color c = canvas->grid[y][x];
            DrawRectangle(
                x + WIDTH / 2 - canvas->width / 2,
                y + HEIGHT / 2 - canvas->height / 2,
                RECT_SIZE,
                RECT_SIZE,
                c
            );
        }
    }
}


/*
C-----D
|     |
|     |
|     |
A-----B
*/

typedef struct {
    Vector3 a, b, c, d;
    Color color;
} Area;

static bool area_is_on_screen(const Area *area) {
    Vector3 perp = Vector3Perpendicular(area->a);
    return perp.z < 0 ? false : true;
}

static void area_into_canvas(Canvas *canvas, const Area *area) {
    float step = 0.01f;
    for (float y=area->a.y; y < area->c.y; y += step) {
        for (float x=area->a.x; x < area->b.x; x += step) {
            size_t index_x = x * canvas->width;
            size_t index_y = (1 - y) * canvas->height;
            canvas->grid[index_y][index_x] = area->color;
        }
    }
}

static void area_into_canvas_lines(Canvas *canvas, const Area *area) {
    Vector3 canvas_dimensions = { canvas->width, canvas->height, 0.0f };

    Vector3 a = Vector3Multiply(vector3_invert_y(area->a), canvas_dimensions);
    Vector3 b = Vector3Multiply(vector3_invert_y(area->b), canvas_dimensions);
    Vector3 c = Vector3Multiply(vector3_invert_y(area->c), canvas_dimensions);
    Vector3 d = Vector3Multiply(vector3_invert_y(area->d), canvas_dimensions);

    canvas_draw_line(canvas, a, b, area->color);
    canvas_draw_line(canvas, a, c, area->color);
    canvas_draw_line(canvas, c, d, area->color);
    canvas_draw_line(canvas, b, d, area->color);
}

static void area_rotate(Area *area, bool rev, Vector3 axis) {
    float angle = 0.1f;
    if (rev)
        angle *= -1.0f;
    area->a = Vector3RotateByAxisAngle(area->a, axis, angle);
    area->b = Vector3RotateByAxisAngle(area->b, axis, angle);
    area->c = Vector3RotateByAxisAngle(area->c, axis, angle);
    area->d = Vector3RotateByAxisAngle(area->d, axis, angle);
}




typedef struct {
    Area areas[6];
    float size;
} Cube;

typedef enum {
    CUBE_BOTTOM = 0,
    CUBE_TOP,
    CUBE_FRONT,
    CUBE_BACK,
    CUBE_LEFT,
    CUBE_RIGHT
} CubeSurface;

static Cube cube_new(Vector3 origin, float size, Color *colors) {
    Cube cube = { 0 };

    Area bottom = {
        origin,
        Vector3Add(origin, (Vector3) { size, 0.0f, 0.0f  }),
        Vector3Add(origin, (Vector3) { 0.0f, 0.0f, -size }),
        Vector3Add(origin, (Vector3) { size, 0.0f, -size }),
        colors[0]
    };
    cube.areas[CUBE_BOTTOM] = bottom;

    Vector3 top_offset = { 0.0f, size, 0.0f };
    Area top = {
        Vector3Add(bottom.a, top_offset),
        Vector3Add(bottom.b, top_offset),
        Vector3Add(bottom.c, top_offset),
        Vector3Add(bottom.d, top_offset),
        colors[1]
    };
    cube.areas[CUBE_TOP] = top;

    Area front = {
        origin,
        Vector3Add(origin, (Vector3) { size, 0.0f, 0.0f }),
        Vector3Add(origin, (Vector3) { 0.0f, size, 0.0f }),
        Vector3Add(origin, (Vector3) { size, size, 0.0f }),
        colors[2]
    };
    cube.areas[CUBE_FRONT] = front;

    Vector3 back_offset = { 0.0f, 0.0f, -size };
    Area back = {
        Vector3Add(front.a, back_offset),
        Vector3Add(front.b, back_offset),
        Vector3Add(front.c, back_offset),
        Vector3Add(front.d, back_offset),
        colors[3]
    };
    cube.areas[CUBE_BACK] = back;

    Area left = {
        Vector3Subtract(origin, (Vector3) { 0.0f, 0.0f, size }),
        origin,
        Vector3Add(origin, (Vector3) { 0.0f, size, -size }),
        Vector3Add(origin, (Vector3) { 0.0f, size, 0.0f }),
        colors[4]
    };
    cube.areas[CUBE_LEFT] = left;

    Vector3 right_offset = { size, 0.0f, 0.0f };
    Area right = {
        Vector3Add(left.a, right_offset),
        Vector3Add(left.b, right_offset),
        Vector3Add(left.c, right_offset),
        Vector3Add(left.d, right_offset),
        colors[5]
    };
    cube.areas[CUBE_RIGHT] = right;

    cube.size = size;
    return cube;
}

static void cube_render_lines(Canvas *canvas, const Cube *cube) {
    for (size_t i=0; i < 6; ++i) {
        // if (!area_is_on_screen(&cube->areas[i]))
        //     continue;
        area_into_canvas_lines(canvas, &cube->areas[i]);
        // area_into_canvas(canvas, &cube->areas[i]);
    }
}

static void cube_rotate(Cube *cube, bool rev) {
    Vector3 axis = cube->areas[CUBE_FRONT].a;

    for (size_t i=0; i < 6; ++i) {
        area_rotate(&cube->areas[i], rev, axis);
    }
}



static void canvas_to_ascii(const Canvas *canvas) {
    for (size_t y=0; y < canvas->height; ++y) {
        for (size_t x=0; x < canvas->width; ++x) {
            Color c = canvas->grid[y][x];
            Color ref = BLACK;
            if (!memcmp(&c, &ref, sizeof (Color)))
                putchar(' ');
            else
                putchar('`');
        }
        puts("");
    }
}


typedef enum {
    MODE_GUI,
    MODE_TUI
    // TODO:
    // mode_xlib,
    // mode_wl,
    // mode_opengl
} Mode;







static void run_gui(Canvas *canvas, Cube *cube) {


    InitWindow(WIDTH, HEIGHT, "cube");
    SetTargetFPS(30);

    while (!WindowShouldClose()) {
        BeginDrawing();
        {
            ClearBackground(BACKGROUND_COLOR);
            canvas_render(canvas);

            cube_rotate(cube, false);

            canvas_fill(canvas, BLACK);
            cube_render_lines(canvas, cube);
        }
        EndDrawing();
    }

    CloseWindow();
}


static void run_tui(Canvas *canvas, Cube *cube) {
    while (true) {
        system("clear");
        cube_rotate(cube, false);
        cube_render_lines(canvas, cube);
        canvas_to_ascii(canvas);
        struct timespec time = { .tv_nsec = 1e8 };
        nanosleep(&time, NULL);
        canvas_fill(canvas, BLACK);
    }
}






int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <mode = gui | tui>\n", argv[0]);
        return EXIT_FAILURE;
    }


    Mode mode;
    if (!strcmp(argv[1], "gui"))
        mode = MODE_GUI;
    else if (!strcmp(argv[1], "tui"))
        mode = MODE_TUI;
    else {
        fprintf(stderr, "Unknown mode: %s\n", argv[1]);
        return EXIT_FAILURE;
    }


    size_t width, height;
    if (mode == MODE_TUI) {
        width  = getmaxx(stdscr);
        height = getmaxy(stdscr);
    } else if (mode == MODE_GUI) {
        // width = GetScreenWidth();
        // height = GetScreenHeight();
        width  = 500;
        height = 500;
    } else
        assert(!"invalid mode");

    Canvas canvas = canvas_new(width, width);
    canvas_fill(&canvas, BLACK);

    Color colors[6] = { 0 };
    colors[CUBE_BOTTOM] = RED;
    colors[CUBE_TOP]    = BLUE;
    colors[CUBE_FRONT]  = ORANGE;
    colors[CUBE_BACK]   = YELLOW;
    colors[CUBE_LEFT]   = GREEN;
    colors[CUBE_RIGHT]  = PURPLE;
    float size     = 0.4f;
    Vector3 origin = { 0.5 - size / 2, 0.5 - size / 2, 0.0 };
    Cube cube      = cube_new(origin, size, colors);


    if (mode == MODE_TUI)
        run_tui(&canvas, &cube);
    else if (mode == MODE_GUI)
        run_gui(&canvas, &cube);
    else
        assert(!"invalid mode");



    canvas_destroy(&canvas);
    return EXIT_SUCCESS;
}
