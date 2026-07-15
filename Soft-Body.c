#include "raylib.h"
#include <math.h>
#include <stdbool.h>

#define WIDTH 800
#define HEIGHT 600

// ------- macros ---------
#define CALC_SPRING(width, height) ((6 * width * height) - (5 * width) - (5 * height) + 2)

//-------Global Variable -------
#define RADIUS 5

#define GRAVITY 400.0f
#define DAMPING 0.99f
#define RESTITUTION 0.8f

#define STIFFNESS 0.05f
#define STRUCT_STIFFNESS 0.8f
#define SHEAR_STIFFNESS  0.4f
#define BEND_STIFFNESS   0.1f

#define SPRING_ITERATION 5


#define OUTER_POINTS 16
#define TOTAL_PARTICLES (OUTER_POINTS + 1)
#define CENTER_IDX OUTER_POINTS // The last slot is the center

// 16 rim springs + 16 spoke springs + 16 structural cross springs
#define TOTAL_SPRINGS (OUTER_POINTS * 3)

#define WHEEL_R_K 0.3f
#define WHEEL_S_K 0.1f
#define WHEEL_C_K 0.05f

// -------- structs --------
typedef struct{
    Vector2 position;
    Vector2 previous_position;
    Vector2 acceleration;
    float mass;
    bool is_pinned;
}Particle;

typedef struct{
    int particle_a;
    int particle_b;
    float rest_length; 
    float stiffness;
}Spring;

// --------- function declarations-----------
void update_particle(Particle *p, float dt);
void spring_calculate(Particle *a, Particle *b, float rest_length, float stiffness);

void draw_particle(Particle p, Color color);
void draw_link_particle(Particle p, Particle pp);

void elastic_border_collision(Particle *p);
void collide_with_obstacle(Particle *p, Vector2 obstacle_pos, float obstacle_radius);

void generate_particle(Particle particles[], int width, int height, Vector2 position, float rest_length, float mass);
void generate_spring_grid(Spring springs[], Particle particles[], int width, int height, float rest_length, float stiffness);

void generate_soft_cirlce(Particle particles[], Spring springs[], Vector2 center, float radius, float mass);

int main()
{
    InitWindow(WIDTH, HEIGHT, "Soft Body Physics");
    SetTargetFPS(60);
    // --------- variable declarations ----------
    const int width   = 10;
    const int height  = 10;
    const int rest_length = 20;
    const int totalParticle = width*height;
    const int totalSprings = CALC_SPRING(width, height);
    
    Particle particles[totalParticle];
    Spring springs[totalSprings];
    generate_particle(particles, width, height,(Vector2){500,50}, rest_length, 10);
    generate_spring_grid(springs, particles, width, height, rest_length, STIFFNESS);

    Particle wheel[TOTAL_PARTICLES];
    Spring wheelSprings[TOTAL_SPRINGS];
    generate_soft_cirlce(wheel, wheelSprings, (Vector2){200,101},100,50);

    //----- game loop ---------
    while(!WindowShouldClose()){
        //---- update everything-------
        //float dt = GetFrameTime();
        static int wheeldraggedIdx = -1;
        static int draggedIdx = -1;
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
            Vector2 mp = GetMousePosition();
            float pick_rad = 50.0f;

            for(int i = 0; i < TOTAL_PARTICLES; i++){
                float dx = wheel[i].position.x - mp.x;
                float dy = wheel[i].position.y - mp.y;
                float dist = sqrt(dx*dx + dy*dy);
                if(dist < pick_rad){
                    wheeldraggedIdx = i;
                }
            }
            for(int i = 0; i < totalParticle; i++){
                float dx = particles[i].position.x - mp.x;
                float dy = particles[i].position.y - mp.y;
                float dist = sqrt(dx*dx + dy*dy);
                if(dist < pick_rad){
                    draggedIdx = i;
                }
            }
        }
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && wheeldraggedIdx != -1){
            Vector2 mp = GetMousePosition();
            wheel[wheeldraggedIdx].position = mp;
            wheel[wheeldraggedIdx].previous_position = mp;
        }
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && draggedIdx != -1){
            Vector2 mp = GetMousePosition();
            particles[draggedIdx].position = mp;
            particles[draggedIdx].previous_position = mp;
        }
        if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)){
            draggedIdx = -1;
            wheeldraggedIdx = -1;
        }

        float dt = 1.0f/60.0f;

        for(int i = 0; i < totalParticle; i++){
            update_particle(&(particles[i]),dt);
            elastic_border_collision(&particles[i]);
        }
        for(int i = 0; i < TOTAL_PARTICLES; i++){
            update_particle(&(wheel[i]),dt);
            elastic_border_collision(&wheel[i]);
        }

        for(int i = 0; i < SPRING_ITERATION; i++){
            for(int j = 0; j < totalSprings; j++){
                spring_calculate(&(particles[springs[j].particle_a]), 
                                &(particles[springs[j].particle_b]),
                                springs[j].rest_length, 
                                springs[j].stiffness
                            );
            }
            for(int k = 0; k < TOTAL_SPRINGS; k++){
                spring_calculate(&(wheel[wheelSprings[k].particle_a]), 
                                &(wheel[wheelSprings[k].particle_b]),
                                wheelSprings[k].rest_length, 
                                wheelSprings[k].stiffness
                            );
            }
        }
        //--------clear and draw --------
        BeginDrawing();
        ClearBackground(BLACK);

        for(int i = 0; i < totalSprings; i++){
            draw_link_particle(particles[springs[i].particle_a], particles[springs[i].particle_b]);
        }
        for(int i = 0; i < TOTAL_SPRINGS; i++){
            draw_link_particle(wheel[wheelSprings[i].particle_a], wheel[wheelSprings[i].particle_b]);
        }
        
        for(int i = 0; i < totalParticle; i++){
            draw_particle(particles[i], RED);
        }
        for(int i = 0; i < TOTAL_PARTICLES; i++){
            draw_particle(wheel[i], PINK);
        }

        EndDrawing();
    }
    return 0;
}
// --------- function definitions -----------
void draw_particle(Particle p, Color color)
{
    DrawCircleV(p.position, RADIUS, color);
}
void draw_link_particle(Particle p, Particle pp)
{
    DrawLineEx(p.position, pp.position, 5, GREEN);
}
void update_particle(Particle *p, float dt)
{
    if(p->is_pinned) return;

    p->acceleration.x = 0;    
    p->acceleration.y = GRAVITY;

    float tx = p->position.x;
    float ty = p->position.y;

    float vx = tx - p->previous_position.x;
    float vy = ty - p->previous_position.y;

    p->position.x = tx + (vx * DAMPING) + (p->acceleration.x * dt * dt);
    p->position.y = ty + (vy * DAMPING) + (p->acceleration.y * dt * dt);

    p->previous_position.x = tx;
    p->previous_position.y = ty;
}

void spring_calculate(Particle *a, Particle *b, float rest_length, float stiffness)
{
    float dx = b->position.x - a->position.x;
    float dy = b->position.y - a->position.y;

    float current_length = sqrt(dx*dx + dy*dy);

    if(current_length <= 0) return;

    float nx = dx/current_length;
    float ny = dy/current_length;
    float diff = current_length - rest_length;
    
    float correctionx = nx * diff * 0.5 * stiffness;
    float correctiony = ny * diff * 0.5 * stiffness;

    if(!a->is_pinned){
        a->position.x += correctionx;
        a->position.y += correctiony;
    }
    if(!b->is_pinned){
        b->position.x -= correctionx;
        b->position.y -= correctiony;
    }
}

void elastic_border_collision(Particle *p)
{
    if(p->position.x + RADIUS >= WIDTH){
        float vx = p->position.x - p->previous_position.x;
        p->position.x = WIDTH - RADIUS;
        p->previous_position.x = p->position.x + (vx * RESTITUTION);
    }
    if(p->position.x - RADIUS <= 0){
        float vx = p->position.x - p->previous_position.x;
        p->position.x = RADIUS;
        p->previous_position.x = p->position.x + (vx * RESTITUTION);
    }
    if(p->position.y + RADIUS >= HEIGHT){
        float vy = p->position.y - p->previous_position.y;
        p->position.y = HEIGHT - RADIUS;
        if(abs(vy/(1.0f/60.0f)) < 0.5){
            p->previous_position.y = p->position.y;
            p->acceleration.y = 0;
        } else {
            p->previous_position.y = p->position.y + (vy * RESTITUTION);
        }
    }
    if(p->position.y - RADIUS <= 0){
        float vy = p->position.y - p->previous_position.y;
        p->position.y = RADIUS;
        p->previous_position.y = p->position.y + (vy * RESTITUTION);
    }
}

void generate_particle(Particle particles[], int width, int height, Vector2 position, float rest_length, float mass)
{
    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            int index = x + (y*width);
            Vector2 pos = {position.x+(x*rest_length), position.y+(y*rest_length)};
            Particle new = {
                .position = pos,
                .previous_position = pos,
                .acceleration = {0},
                .mass = mass,
                .is_pinned = false
            };
            particles[index] = new;
        }
    }
}

void generate_spring_grid(Spring springs[], Particle particles[], int width, int height, float rest_length, float stiffness)
{
    int count = 0;
    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            int currentIdx = x + (y*width);
            //-- structural spring --
            //Right neighbor
            if(x < width - 1){
                int neighborIdx = (x+1) + (y*width);

                float dx = particles[neighborIdx].position.x - particles[currentIdx].position.x;
                float dy = particles[neighborIdx].position.y - particles[currentIdx].position.y;
                float rest_length = sqrt(dx*dx + dy*dy); 

                springs[count++] = (Spring){
                    .particle_a = currentIdx,
                    .particle_b = neighborIdx,
                    .rest_length = rest_length,
                    .stiffness = STRUCT_STIFFNESS
                };
            }
            // Down neighbor
            if(y <  height - 1){
                int neighborIdx = x + ((y+1) * width);

                float dx = particles[neighborIdx].position.x - particles[currentIdx].position.x;
                float dy = particles[neighborIdx].position.y - particles[currentIdx].position.y;
                float rest_length = sqrt(dx*dx + dy*dy); 

                springs[count++] = (Spring){
                    .particle_a = currentIdx,
                    .particle_b = neighborIdx,
                    .rest_length = rest_length,
                    .stiffness = STRUCT_STIFFNESS
                };
            }

            // ------ shear springs --------
            // Down-Right
            if(x < width - 1 && y <  height - 1){
                int neighborIdx = (x + 1) + ((y+1) * width);

                float dx = particles[neighborIdx].position.x - particles[currentIdx].position.x;
                float dy = particles[neighborIdx].position.y - particles[currentIdx].position.y;
                float rest_length = sqrt(dx*dx + dy*dy); 

                springs[count++] = (Spring){
                    .particle_a = currentIdx,
                    .particle_b = neighborIdx,
                    .rest_length = rest_length,
                    .stiffness = SHEAR_STIFFNESS
                };
            }
            //Down-left
            if(x > 0 && y < height - 1){
                int neighborIdx = (x - 1) + ((y + 1) * width);

                float dx = particles[neighborIdx].position.x - particles[currentIdx].position.x;
                float dy = particles[neighborIdx].position.y - particles[currentIdx].position.y;
                float rest_length = sqrt(dx*dx + dy*dy); 

                springs[count++] = (Spring){
                    .particle_a = currentIdx,
                    .particle_b = neighborIdx,
                    .rest_length = rest_length,
                    .stiffness = SHEAR_STIFFNESS
                };
            }

            // ---- Bend spring -----
            // Tow units right
            if(x <  width - 2){
                int neighborIdx = (x + 2) + (y * width);

                float dx = particles[neighborIdx].position.x - particles[currentIdx].position.x;
                float dy = particles[neighborIdx].position.y - particles[currentIdx].position.y;
                float rest_length = sqrt(dx*dx + dy*dy); 

                springs[count++] = (Spring){
                    .particle_a = currentIdx,
                    .particle_b = neighborIdx,
                    .rest_length = rest_length,
                    .stiffness = BEND_STIFFNESS
                };
            }
            // Two units Down
            if(y <  height - 2){
                int neighborIdx = x + ((y+2) * width);

                float dx = particles[neighborIdx].position.x - particles[currentIdx].position.x;
                float dy = particles[neighborIdx].position.y - particles[currentIdx].position.y;
                float rest_length = sqrt(dx*dx + dy*dy); 

                springs[count++] = (Spring){
                    .particle_a = currentIdx,
                    .particle_b = neighborIdx,
                    .rest_length = rest_length,
                    .stiffness = BEND_STIFFNESS
                };
            }
        }
    }
}


void generate_soft_cirlce(Particle particles[], Spring springs[], Vector2 center, float radius, float mass)
{
   // generate particles 
   // first, place the outer ring particles
   for(int i = 0; i < OUTER_POINTS; i++){
        float angle = i * (2.0f*PI/ OUTER_POINTS);
        Vector2 pos = {
            center.x + cosf(angle) * radius,
            center.y + sinf(angle) * radius
        };
        particles[i] = (Particle){
            .position = pos,
            .previous_position = pos,
            .acceleration = {0},
            .mass = mass,
            .is_pinned = false
        };
   }
    // place the center particle at the very end of the array
    particles[CENTER_IDX] = (Particle){
        .position = center,
        .previous_position = center,
        .acceleration = {0},
        .mass = mass,
        .is_pinned = false
    };

    // Generate Springs 
    int springCount = 0;
    for(int i = 0; i < OUTER_POINTS; i++){
        int next = (i + 1) % OUTER_POINTS; // Next neighbor on the rim
        int skip = (i + 2) % OUTER_POINTS; // skip one neighbor for shear stiffness

        // -- type 1: The rim (Outer skin) --
        float dx = particles[i].position.x - particles[next].position.x;
        float dy = particles[i].position.y - particles[next].position.y;
        springs[springCount++] = (Spring){
            .particle_a = i,
            .particle_b = next,
            .rest_length = sqrt(dx*dx + dy*dy),
            .stiffness = WHEEL_R_K 
        };
        //type 1 : the spokes (connected to center)
        springs[springCount++] = (Spring){
            .particle_a = i,
            .particle_b = CENTER_IDX, 
            .rest_length = radius,
            .stiffness = WHEEL_S_K
        };
        //type 3: cross-braces --
        float ddx = particles[i].position.x - particles[skip].position.x;
        float ddy = particles[i].position.y - particles[skip].position.y;
        springs[springCount++] = (Spring){
            .particle_a = i,
            .particle_b = skip,
            .rest_length = sqrt(ddx*ddx + ddy*ddy),
            .stiffness = WHEEL_C_K 
        };
    }
}

void collide_with_obstacle(Particle *p, Vector2 obstacle_pos, float obstacle_radius) 
{
    float dx = p->position.x - obstacle_pos.x;
    float dy = p->position.y - obstacle_pos.y;
    float dist = sqrtf(dx*dx + dy*dy);
    float min_dist = obstacle_radius + RADIUS;

    if (dist < min_dist) {
        float nx = dx / dist;
        float ny = dy / dist;
        float overlap = min_dist - dist;

        // Push particle out of the obstacle
        p->position.x += nx * overlap;
        p->position.y += ny * overlap;

        // Simple velocity dampening on bounce
        float vx = p->position.x - p->previous_position.x;
        float vy = p->position.y - p->previous_position.y;
        p->previous_position.x = p->position.x + (vx * 0.5f);
        p->previous_position.y = p->position.y + (vy * 0.5f);
    }
}
