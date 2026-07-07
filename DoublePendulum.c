#include "raylib.h"
#include <math.h>

#define WIDTH 800 
#define HEIGHT 600

//---------------- Global constants -------------
#define GRAVITY 500

#define THICKNESS 8 
#define MAX_TRAIL 50

//----------- Structs --------------
typedef struct {
    Vector2 position;
    float mass;
    float angle;
    float angular_velocity;
    float length;
}Pendulum;

// --------------- function declaration ---------------
void draw_double_pendulum(Pendulum *p, Pendulum *pp, float dt);

int main()
{
    InitWindow(WIDTH, HEIGHT, "Double Pendulum");
    SetTargetFPS(60);
    // ------------- Variable declarations --------------
    Pendulum a    = {
        .position = (Vector2){400, 10},
        .mass     = 100.0f,
        .angle    = 45.0f * DEG2RAD,
        .angular_velocity = 0.0f,
        .length   = 100.0f
    };
    Pendulum b    = {
        .position = (Vector2){0.0f, 0.0f},
        .mass     = 100.0f,
        .angle    = 250.0f * DEG2RAD,
        .angular_velocity = 0.0f,
        .length   = 200.0f
    };

    Vector2 trail[MAX_TRAIL] = {0};
    size_t trail_count = 0;
    size_t trail_index = 0;
    float frame_count = 0;

    // ------------ Game Loop ----------------
    while(!WindowShouldClose()){
        //--------------- update everything ---------
        float dt = GetFrameTime();

        //update trail
        Vector2 end = {a.position.x + a.length * sin(a.angle),
                       a.position.y + a.length * cos(a.angle) 
                      };
        Vector2 ppend = {end.x + b.length * sin(b.angle),
                         end.y + b.length * cos(b.angle) 
                      };

        frame_count += dt;
        if(frame_count >= 0.05f){
            if(trail_count < MAX_TRAIL){
                trail[trail_count % MAX_TRAIL] = ppend;
                trail_count++;
            }
            else{
                trail[trail_index % MAX_TRAIL] = ppend;
                if(trail_index < MAX_TRAIL) trail_index++;
                else trail_index = 0;
            }
            frame_count -= 0.05f;
        }

        // ------------------ draw everything ---------------
        BeginDrawing();
        ClearBackground(BLACK);

        for(int i = 0; i < trail_count; i++) DrawCircleV(trail[i], THICKNESS-3, RED);
        draw_double_pendulum(&a, &b, dt);

        EndDrawing();
    }
    // ------------ close everything -----------------
    CloseWindow();
    return 0;
}
// ------------ function definition ---------------

void draw_double_pendulum(Pendulum *p, Pendulum *pp, float dt)
{
    //angle in radians
    float r1 = p->angle;
    float r2 = pp->angle;

    // calculate the new anglular acceleration
    float num1 = -GRAVITY * (2 * p->mass + pp->mass) * sin(r1) - pp->mass * GRAVITY * sin(r1 - 2 * r2) - 2 * sin(r1 - r2) * pp->mass * (pp->angular_velocity * pp->angular_velocity * pp->length + p->angular_velocity * p->angular_velocity * p->length * cos(r1 - r2));
    float den1 = p->length * (2 * p->mass + pp->mass - pp->mass * cos(2 * r1 - 2 * r2));
    float angular_accel1 = num1 / den1;

    float num2 = 2 * sin(r1 - r2) * (p->angular_velocity * p->angular_velocity * p->length * (p->mass + pp->mass) + GRAVITY * (p->mass + pp->mass) * cos(r1) + pp->angular_velocity * pp->angular_velocity * pp->length * pp->mass * cos(r1 - r2));
    float den2 = pp->length * (2 * p->mass + pp->mass - pp->mass * cos(2 * r1 - 2 * r2));
    float angular_accel2 = num2 / den2;

    // update the angle
    p->angular_velocity += angular_accel1 * dt;
    pp->angular_velocity += angular_accel2 * dt;

    p->angle += p->angular_velocity * dt;
    pp->angle += pp->angular_velocity * dt;
    
    //calculate the ending p->sition 
    float radp = p->angle;
    Vector2 end = {p->position.x + p->length * sin(radp),
                   p->position.y + p->length * cos(radp) 
                  };
    float radpp = pp->angle;
    Vector2 ppend = {end.x + pp->length * sin(radpp),
                     end.y + pp->length * cos(radpp) 
                  };
    //draw 
    DrawLineEx(p->position, end, THICKNESS, WHITE);
    DrawCircleV(p->position, THICKNESS, RED);

    DrawLineEx(end, ppend, THICKNESS, WHITE);
    DrawCircleV(end, THICKNESS+3, RED);
    DrawCircleV(ppend, THICKNESS+8, RED);
}

