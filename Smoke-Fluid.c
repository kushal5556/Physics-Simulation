#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define WIDTH  800
#define HEIGHT 600

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define GRID_WIDTH 60
#define GRID_HEIGHT 45

typedef struct {
    Vector2 velocity;
    float density;
    bool is_obstacle;  
} Cell;

// --- function declarations ----
void init_grid(Cell grid[]);
void update_physics(Cell grid[], float dt);
void emit_fluid(Cell grid[], float pixel_x, float pixel_y, float radius, float amount, Vector2 push_vel);

int main()
{
    InitWindow(WIDTH, HEIGHT, "Advanced Smoke & Fluid Simulation");
    SetTargetFPS(60);

    Cell grid[GRID_WIDTH * GRID_HEIGHT];
    init_grid(grid);

    // ----- game loop ----
    while(!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f; 

        // 1. CUSTOM POSITION EMITTER (Center-Bottom Chimney Plume)
        emit_fluid(grid, 400.0f, 550.0f, 2.5f, 255.0f, (Vector2){0.0f, -180.0f});

        // 2. INTERACTION (Left Click = Add Smoke | Right Click = Place Walls)
        Vector2 mousePos = GetMousePosition();
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 drag = GetMouseDelta();
            emit_fluid(grid, mousePos.x, mousePos.y, 1.5f, 255.0f, (Vector2){drag.x * 4.0f, drag.y * 4.0f});
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            int mx = (int)(mousePos.x / ((float)WIDTH / GRID_WIDTH));
            int my = (int)(mousePos.y / ((float)HEIGHT / GRID_HEIGHT));
            if (mx > 0 && mx < GRID_WIDTH - 1 && my > 0 && my < GRID_HEIGHT - 1) {
                grid[mx + (my * GRID_WIDTH)].is_obstacle = true;
            }
        }

        // ---- update physics -----
        update_physics(grid, dt);

        // -- clear and draw --
        BeginDrawing();
        ClearBackground((Color){10, 10, 15, 255}); 
        
        float cell_w = (float)WIDTH / GRID_WIDTH;
        float cell_h = (float)HEIGHT / GRID_HEIGHT;

        for(int y = 0; y < GRID_HEIGHT; y++) {
            for(int x = 0; x < GRID_WIDTH; x++) {
                int idx = x + (y * GRID_WIDTH);

                if (grid[idx].is_obstacle) {
                    DrawRectangleRec((Rectangle){ x * cell_w, y * cell_h, cell_w, cell_h }, (Color){70, 90, 110, 255});
                    continue;
                }

                float d = grid[idx].density;
                if (d <= 0.5f) continue; 

                // ---- COLOR SHADER GRADIENT ----
                Color renderColor;
                float t = d / 255.0f; 

                if (t > 0.6f) {
                    float internal_t = (t - 0.6f) / 0.4f;
                    renderColor.r = (unsigned char)(255);
                    renderColor.g = (unsigned char)(200 + (55 * internal_t));
                    renderColor.b = (unsigned char)(100 + (155 * internal_t));
                } else if (t > 0.2f) {
                    float internal_t = (t - 0.2f) / 0.4f;
                    renderColor.r = (unsigned char)(120 + (135 * internal_t));
                    renderColor.g = (unsigned char)(40 + (160 * internal_t));
                    renderColor.b = (unsigned char)(40 + (60 * internal_t));
                } else {
                    float internal_t = t / 0.2f;
                    renderColor.r = (unsigned char)(40 + (80 * internal_t));
                    renderColor.g = (unsigned char)(40 + (80 * internal_t));
                    renderColor.b = (unsigned char)(50 + (70 * internal_t));
                }
                
                renderColor.a = (unsigned char)(MIN(d * 2.0f, 255.0f)); 

                DrawRectangleRec((Rectangle){ x * cell_w, y * cell_h, cell_w, cell_h }, renderColor);
            }
        }

        DrawText("Left-Click: Spray Smoke | Right-Click: Draw Solid Obstacles", 10, 10, 18, RAYWHITE);
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}

void init_grid(Cell grid[])
{
    for(int y = 0; y < GRID_HEIGHT; y++) {
        for(int x = 0; x < GRID_WIDTH; x++) {
            int idx = x + (y * GRID_WIDTH);
            grid[idx].velocity = (Vector2){0.0f, 0.0f};
            grid[idx].density = 0.0f;
            grid[idx].is_obstacle = false;

            // Places a default target block right in the center field path
            float cx = GRID_WIDTH / 2.0f;
            float cy = GRID_HEIGHT / 2.5f;
            float r = 4.5f; 
            if (sqrtf((x - cx)*(x - cx) + (y - cy)*(y - cy)) < r) {
                grid[idx].is_obstacle = true;
            }
        }
    }
}

void emit_fluid(Cell grid[], float pixel_x, float pixel_y, float radius, float amount, Vector2 push_vel)
{
    int target_x = (int)(pixel_x / ((float)WIDTH / GRID_WIDTH));
    int target_y = (int)(pixel_y / ((float)HEIGHT / GRID_HEIGHT));

    for (int y = -(int)radius; y <= (int)radius; y++) {
        for (int x = -(int)radius; x <= (int)radius; x++) {
            int gx = target_x + x;
            int gy = target_y + y;

            if (gx > 0 && gx < GRID_WIDTH - 1 && gy > 0 && gy < GRID_HEIGHT - 1) {
                if (x*x + y*y <= radius*radius) { 
                    int idx = gx + (gy * GRID_WIDTH);
                    if (!grid[idx].is_obstacle) {
                        grid[idx].density = amount;
                        grid[idx].velocity.x += push_vel.x * 0.4f;
                        grid[idx].velocity.y += push_vel.y * 0.4f;
                    }
                }
            }
        }
    }
}

void update_physics(Cell grid[], float dt)
{
    int size = GRID_WIDTH * GRID_HEIGHT;
    Cell next_grid[GRID_WIDTH * GRID_HEIGHT];
    memcpy(next_grid, grid, sizeof(Cell) * size);

    // --- STEP 1: ADVECTION ---
    for (int y = 1; y < GRID_HEIGHT - 1; y++) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            int idx = x + (y * GRID_WIDTH);
            if (grid[idx].is_obstacle) continue;

            float old_x = (float)x - (grid[idx].velocity.x * dt);
            float old_y = (float)y - (grid[idx].velocity.y * dt);

            if (old_x < 0.5f) old_x = 0.5f;
            if (old_x > GRID_WIDTH - 1.5f) old_x = GRID_WIDTH - 1.5f;
            if (old_y < 0.5f) old_y = 0.5f;
            if (old_y > GRID_HEIGHT - 1.5f) old_y = GRID_HEIGHT - 1.5f;

            int x0 = (int)old_x; int x1 = x0 + 1;
            int y0 = (int)old_y; int y1 = y0 + 1;

            float sx = old_x - (float)x0;
            float sy = old_y - (float)y0;

            next_grid[idx].density = (1-sx)*(1-sy)*grid[x0 + (y0 * GRID_WIDTH)].density + sx*(1-sy)*grid[x1 + (y0 * GRID_WIDTH)].density +
                                     (1-sx)*sy*grid[x0 + (y1 * GRID_WIDTH)].density     + sx*sy*grid[x1 + (y1 * GRID_WIDTH)].density;

            next_grid[idx].velocity.x = (1-sx)*(1-sy)*grid[x0 + (y0 * GRID_WIDTH)].velocity.x + sx*(1-sy)*grid[x1 + (y0 * GRID_WIDTH)].velocity.x +
                                        (1-sx)*sy*grid[x0 + (y1 * GRID_WIDTH)].velocity.x     + sx*sy*grid[x1 + (y1 * GRID_WIDTH)].velocity.x;

            next_grid[idx].velocity.y = (1-sx)*(1-sy)*grid[x0 + (y0 * GRID_WIDTH)].velocity.y + sx*(1-sy)*grid[x1 + (y0 * GRID_WIDTH)].velocity.y +
                                        (1-sx)*sy*grid[x0 + (y1 * GRID_WIDTH)].velocity.y     + sx*sy*grid[x1 + (y1 * GRID_WIDTH)].velocity.y;
            
            next_grid[idx].density *= 0.985f; 

            // Thermal buoyancy: gas accelerates upwards based on current grid density
            if (grid[idx].density > 1.0f) {
                next_grid[idx].velocity.y -= (0.12f * grid[idx].density) * dt;
            }
        }
    }
    memcpy(grid, next_grid, sizeof(Cell) * size);

    // --- STEP 2: INCOMPRESSIBILITY / PROJECTION ---
    float p[GRID_WIDTH * GRID_HEIGHT] = {0};
    float div[GRID_WIDTH * GRID_HEIGHT] = {0};

    for (int y = 1; y < GRID_HEIGHT - 1; y++) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            int idx = x + (y * GRID_WIDTH);
            if (grid[idx].is_obstacle) continue;

            float vR = grid[idx + 1].is_obstacle ? -grid[idx].velocity.x : grid[idx + 1].velocity.x;
            float vL = grid[idx - 1].is_obstacle ? -grid[idx].velocity.x : grid[idx - 1].velocity.x;
            float vT = grid[idx + GRID_WIDTH].is_obstacle ? -grid[idx].velocity.y : grid[idx + GRID_WIDTH].velocity.y;
            float vB = grid[idx - GRID_WIDTH].is_obstacle ? -grid[idx].velocity.y : grid[idx - GRID_WIDTH].velocity.y;

            div[idx] = -0.5f * (vR - vL + vT - vB);
        }
    }

    for (int k = 0; k < 25; k++) {
        for (int y = 1; y < GRID_HEIGHT - 1; y++) {
            for (int x = 1; x < GRID_WIDTH - 1; x++) {
                int idx = x + (y * GRID_WIDTH);
                if (grid[idx].is_obstacle) continue;

                float pR = grid[idx + 1].is_obstacle ? p[idx] : p[idx + 1];
                float pL = grid[idx - 1].is_obstacle ? p[idx] : p[idx - 1];
                float pT = grid[idx + GRID_WIDTH].is_obstacle ? p[idx] : p[idx + GRID_WIDTH];
                float pB = grid[idx - GRID_WIDTH].is_obstacle ? p[idx] : p[idx - GRID_WIDTH];

                p[idx] = (div[idx] + pL + pR + pB + pT) / 4.0f;
            }
        }
    }

    for (int y = 1; y < GRID_HEIGHT - 1; y++) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            int idx = x + (y * GRID_WIDTH);
            if (grid[idx].is_obstacle) continue;

            if (!grid[idx + 1].is_obstacle && !grid[idx - 1].is_obstacle)
                grid[idx].velocity.x -= 0.5f * (p[idx + 1] - p[idx - 1]);
            if (!grid[idx + GRID_WIDTH].is_obstacle && !grid[idx - GRID_WIDTH].is_obstacle)
                grid[idx].velocity.y -= 0.5f * (p[idx + GRID_WIDTH] - p[idx - GRID_WIDTH]);
        }
    }

    // --- STEP 3: BOUNDARY ENFORCEMENT ---
    for (int x = 0; x < GRID_WIDTH; x++) {
        grid[x].velocity.y = -grid[x + GRID_WIDTH].velocity.y;

        grid[x + (GRID_HEIGHT - 1) * GRID_WIDTH].velocity.y = -grid[x + (GRID_HEIGHT - 2) * GRID_WIDTH].velocity.y;
    }
    for (int y = 0; y < GRID_HEIGHT; y++) {
        grid[y * GRID_WIDTH].velocity.x = -grid[1 + y * GRID_WIDTH].velocity.x;
        grid[(GRID_WIDTH - 1) + y * GRID_WIDTH].velocity.x = -grid[(GRID_WIDTH - 2) + y * GRID_WIDTH].velocity.x;
    }
    // Force zero fluid values inside static wall masks
    for (int y = 1; y < GRID_HEIGHT - 1; y++) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            int idx = x + (y * GRID_WIDTH);
            if (grid[idx].is_obstacle) {
                grid[idx].velocity = (Vector2){0.0f, 0.0f};grid[idx].density = 0.0f;
            }
        }
    }
}







