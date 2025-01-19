#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include <raylib.h>
#include <raymath.h>
#include <time.h>


#define WIDTH 1920
#define HEIGHT 1080
#define BACKGROUND_COLOR (Color) { 51, 51, 51, 255 }

// #define CANVAS_WIDTH 1000
#define CANVAS_WIDTH 50
#define CANVAS_HEIGHT CANVAS_WIDTH
#define RECT_SIZE 1

typedef Color Canvas[CANVAS_HEIGHT][CANVAS_WIDTH];


static void vector3_print(Vector3 v) {
    printf("x = %f, y = %f, z = %f\n", v.x, v.y, v.z);
}

static Vector3 vector3_invert_y(Vector3 v) {
    v.y = 1.0f - v.y;
    return v;
}

static void draw_line(Canvas canvas, Vector3 start, Vector3 end, Color color) {
    end.x += 1; // cannot draw perfect straight line, as end.x - start-x == 0
    float slope = (end.y - start.y) / (end.x - start.x);
    float step = 0.01f;
    for (float x=start.x; x < end.x; x+=step) {
        float y = slope * (x - start.x) + start.y;
        if (y > CANVAS_HEIGHT)
            continue;
        canvas[(size_t)y][(size_t)x] = color;
    }
}



static void canvas_fill(Canvas canvas, Color color) {
    for (size_t y=0; y < CANVAS_HEIGHT; ++y) {
        for (size_t x=0; x < CANVAS_WIDTH; ++x) {
            canvas[y][x] = color;
        }
    }
}

static void canvas_render(Canvas canvas) {
    for (size_t y=0; y < CANVAS_HEIGHT; ++y) {
        for (size_t x=0; x < CANVAS_WIDTH; ++x) {
            Color c = canvas[y][x];
            DrawRectangle(
                x + WIDTH / 2 - CANVAS_WIDTH / 2,
                y + HEIGHT / 2 - CANVAS_HEIGHT / 2,
                RECT_SIZE,
                RECT_SIZE,
                c
            );
        }
    }
}

static void vec_to_canvas(Canvas canvas, Vector3 v, Color color) {
    size_t y = (1.0f - v.y) * CANVAS_HEIGHT;
    size_t x = v.x * CANVAS_WIDTH;
    canvas[y][x] = color;
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

static void area_into_canvas(Canvas canvas, const Area *area) {
    float step = 0.01f;
    for (float y=area->a.y; y < area->c.y; y += step) {
        for (float x=area->a.x; x < area->b.x; x += step) {
            size_t index_x = x * CANVAS_WIDTH;
            size_t index_y = (1 - y) * CANVAS_HEIGHT;

            canvas[index_y][index_x] = area->color;

        }
    }
}

static void area_into_canvas_lines(Canvas canvas, const Area *area) {
    Vector3 canvas_dimensions = { CANVAS_WIDTH, CANVAS_HEIGHT, 0.0f };

    draw_line(
        canvas,
        Vector3Multiply(vector3_invert_y(area->a), canvas_dimensions),
        Vector3Multiply(vector3_invert_y(area->b), canvas_dimensions),
        area->color
    );

    draw_line(
        canvas,
        Vector3Multiply(vector3_invert_y(area->a), canvas_dimensions),
        Vector3Multiply(vector3_invert_y(area->c), canvas_dimensions),
        area->color
    );

    draw_line(
        canvas,
        Vector3Multiply(vector3_invert_y(area->c), canvas_dimensions),
        Vector3Multiply(vector3_invert_y(area->d), canvas_dimensions),
        area->color
    );

    draw_line(
        canvas,
        Vector3Multiply(vector3_invert_y(area->b), canvas_dimensions),
        Vector3Multiply(vector3_invert_y(area->d), canvas_dimensions),
        area->color
    );

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

static Cube cube_new(Vector3 origin, float size) {
    Cube cube = { 0 };

    Area bottom = {
        origin,
        Vector3Add(origin, (Vector3) { size, 0.0f, 0.0f }),
        Vector3Add(origin, (Vector3) { 0.0f, 0.0f, -size }),
        Vector3Add(origin, (Vector3) { size, 0.0f, -size }),
        RED
    };
    cube.areas[CUBE_BOTTOM] = bottom;

    Vector3 top_offset = { 0.0f, size, 0.0f };
    Area top = {
        Vector3Add(bottom.a, top_offset),
        Vector3Add(bottom.b, top_offset),
        Vector3Add(bottom.c, top_offset),
        Vector3Add(bottom.d, top_offset),
        BLUE
    };
    cube.areas[CUBE_TOP] = top;

    Area front = {
        origin,
        Vector3Add(origin, (Vector3) { size, 0.0f, 0.0f }),
        Vector3Add(origin, (Vector3) { 0.0f, size, 0.0f }),
        Vector3Add(origin, (Vector3) { size, size, 0.0f }),
        GREEN
    };
    cube.areas[CUBE_FRONT] = front;

    Vector3 back_offset = { 0.0f, 0.0f, -size };
    Area back = {
        Vector3Add(front.a, back_offset),
        Vector3Add(front.b, back_offset),
        Vector3Add(front.c, back_offset),
        Vector3Add(front.d, back_offset),
        YELLOW
    };
    cube.areas[CUBE_BACK] = back;

    Area left = {
        Vector3Subtract(origin, (Vector3) { 0.0f, 0.0f, size }),
        origin,
        Vector3Add(origin, (Vector3) { 0.0f, size, -size }),
        Vector3Add(origin, (Vector3) { 0.0f, size, 0.0f }),
        ORANGE
    };
    cube.areas[CUBE_LEFT] = left;

    Vector3 right_offset = { size, 0.0f, 0.0f };
    Area right = {
        Vector3Add(left.a, right_offset),
        Vector3Add(left.b, right_offset),
        Vector3Add(left.c, right_offset),
        Vector3Add(left.d, right_offset),
        PURPLE
    };
    cube.areas[CUBE_RIGHT] = right;

    return cube;
}

static void cube_render_lines(Canvas canvas, const Cube *cube) {
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



static void canvas_to_ascii(Canvas canvas) {
    for (size_t y=0; y < CANVAS_HEIGHT; ++y) {
        for (size_t x=0; x < CANVAS_WIDTH; ++x) {
            Color c = canvas[y][x];
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

int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <gui | tui>", argv[0]);
        return EXIT_FAILURE;
    }

    const char *mode = argv[1];

    if (!strcmp(mode, "gui")) {
    }



    Canvas canvas = { 0 };
    canvas_fill(canvas, BLACK);

    float size = 0.3f;
    Cube cube = cube_new((Vector3){ 0.5-size/2, 0.5-size/2, 0.0 }, size);

    while (true) {
        system("clear");
        cube_rotate(&cube, false);
        cube_render_lines(canvas, &cube);
        canvas_to_ascii(canvas);
        struct timespec time = { .tv_nsec = 1e8 };
        nanosleep(&time, NULL);
        canvas_fill(canvas, BLACK);
    }
    return 0;

    InitWindow(WIDTH, HEIGHT, "cube");

    while (!WindowShouldClose()) {
        BeginDrawing();
        {
            ClearBackground(BACKGROUND_COLOR);
            canvas_render(canvas);

            cube_rotate(&cube, false);

            canvas_fill(canvas, BLACK);
            cube_render_lines(canvas, &cube);
        }
        EndDrawing();
    }

    CloseWindow();
    return EXIT_SUCCESS;
}
