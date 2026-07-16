#include "raylib.h"
#include <math.h>

#define WIDTH 1200
#define HEIGHT 800 

// ------ global  constant-------
#define SOFTENING 0.5f
#define G 1
#define TIME_STEP 0.016f // speed of simulation 
#define MAX_SPEED 10000 * 2

#define TOTAL_PARTICLE 100 

// ----- macros ---------
#define G_F(m1, m2, r) (G * (m1 * m2) / (r*r))

// ---- structs --------
typedef struct{
    Vector2 position;
    Vector2 velocity;
    float mass;
    float radius;
    Color color;
}Particle;

//----- function declarations --------
void update_Particle(Particle *p, float dt);
void update_acceleration(Particle *a, Particle *b);

void random_Particle(Particle Particles[]);

int main()
{
    InitWindow(WIDTH, HEIGHT, "n body gravity");
    SetTargetFPS(60);
    // ------ variable declarations ------
    Particle Particles[TOTAL_PARTICLE+1];
    random_Particle(Particles);

    Particle center = {
        .position = (Vector2){WIDTH/2,HEIGHT/2},
        .velocity = (Vector2){0,0},
        .mass = 5000,
        .radius = 15,
        .color = RED
    };
    Particles[TOTAL_PARTICLE] = center;

    // ---- game loop -----
    while(!WindowShouldClose()){
        //----- update everything ---
        float dt = GetFrameTime();
        
        for(int i = 0; i < TOTAL_PARTICLE; i++){
            for(int j = 0; j < TOTAL_PARTICLE+1; j++){
                if(i == j) continue;
                update_acceleration(&Particles[i], &Particles[j]);
            }
        }
        for(int i = 0; i < TOTAL_PARTICLE+1; i++){
            update_Particle(&Particles[i], dt);
        }

        // ---- clear and draw -----
        BeginDrawing();
        ClearBackground(BLACK);

        for(int i = 0; i < TOTAL_PARTICLE+1; i++){
            DrawCircleV(Particles[i].position, Particles[i].radius,Particles[i].color);
        }

        EndDrawing();
    }
    // ---- close -------
    CloseWindow();
    return 0;
}
// --- function definition ------
void update_Particle(Particle *p, float dt)
{
    float speed = sqrt(p->velocity.x*p->velocity.x + p->velocity.y*p->velocity.y);
    if(speed > MAX_SPEED){
        p->velocity.x = (p->velocity.x/speed) * MAX_SPEED;
        p->velocity.y = (p->velocity.y/speed) * MAX_SPEED;
    }
    p->position.x += p->velocity.x * dt * TIME_STEP;
    p->position.y += p->velocity.y * dt * TIME_STEP;
}
void update_acceleration(Particle *a, Particle *b)
{
    float dx = b->position.x - a->position.x;
    float dy = b->position.y - a->position.y;

    float dist = sqrt(dx*dx + dy*dy);

    float nx = dx/dist;
    float ny = dy/dist;

    float force = G_F(a->mass, b->mass, dist+SOFTENING);

    a->velocity.x += nx * (force/a->mass); 
    a->velocity.y += ny * (force/a->mass);
}

void random_Particle(Particle Particles[])
{
    for(int i = 0; i < TOTAL_PARTICLE; i++){
        float radius = GetRandomValue(3,8);

        Particle new = {
            .position = (Vector2){GetRandomValue(0, WIDTH), GetRandomValue(0, HEIGHT)},
            .velocity = (Vector2){GetRandomValue(-5,5),GetRandomValue(-5,5)},
//            .mass = GetRandomValue(40,50) * radius,
            .mass = 100,
            .radius = radius,
            .color = (Color){
                GetRandomValue(150,255),
                GetRandomValue(150,255),
                GetRandomValue(150,255),
                GetRandomValue(150,255)
            }
        };
        Particles[i] = new;
    }
}



