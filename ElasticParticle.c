#include "raylib.h"
#include <math.h>

#define WIDTH 800
#define HEIGHT 600

// -------- Global Variable -----------
#define OBJ_COUNT 20

#define RESTITUTION 1

//---------- structs ---------------
typedef struct {
    Vector2 position;
    Vector2 previous_position;
    Vector2 acceleration;
    float mass;
}Object;

typedef struct {
    Object body;
    float radius;
    Color color;
}Obj_circle;

// ----------- Fucntion Declarations -------------
void random_object(Obj_circle arr[], int count);
void draw_obj_circle(Obj_circle arr[], int count);
void update_obj_circle(Obj_circle arr[], int count, float dt);
void edge_collision_obj_circle(Obj_circle *obj);

void elastic_collision_obj_circle(Obj_circle *a, Obj_circle *b, float dt);

int main()
{
    InitWindow(WIDTH, HEIGHT, "Elastic Particle");
    SetTargetFPS(60);

    //------------ Variable declarations -------------
    Obj_circle objects[OBJ_COUNT] = {0}; 
    random_object(objects, OBJ_COUNT);

    //------------------- Game Loop ----------------
    while(!WindowShouldClose()){
        // ----------- update everything ------------
        float dt = GetFrameTime();
        update_obj_circle(objects, OBJ_COUNT, dt);

        for(int i = 0; i < OBJ_COUNT; i++){
            for(int j = i+1; j < OBJ_COUNT; j++){
                elastic_collision_obj_circle(&objects[i], &objects[j], dt);
            }
        }

        // ------------ draw ---------------
        BeginDrawing();
        ClearBackground(BLACK);

        draw_obj_circle(objects, OBJ_COUNT);

        EndDrawing();
    }
    //----------- Close everything -------------
    return 0;
}
// ----------- Function definitions -----------
void random_object(Obj_circle arr[], int count)
{
    for(int i = 0; i < count; i++){
        float radius = GetRandomValue(10,20);
        Vector2 position = {GetRandomValue(radius, WIDTH-radius), 
                            GetRandomValue(radius, HEIGHT-radius)
                        };

        Obj_circle new = {
            .body = {
                .position = position,
                .previous_position = {0},
                .acceleration = {0},
                .mass = (float)GetRandomValue(30,100)
            },
            .radius = radius,
            .color  = (Color){GetRandomValue(150,255), GetRandomValue(100,255),
                              GetRandomValue(100,255), GetRandomValue(200,255)}
        };

        float dt = 1.0f/60.0f;
        float vx = (float)GetRandomValue(-50,50);
        float vy = (float)GetRandomValue(-50,50);

        new.body.previous_position.x = new.body.position.x - (vx * dt);
        new.body.previous_position.y = new.body.position.y - (vy * dt);

        arr[i] = new;
    }
}

void draw_obj_circle(Obj_circle arr[], int count)
{
    for(int i = 0; i < count; i++){
        DrawCircleV(arr[i].body.position, arr[i].radius, arr[i].color);
    }
}

void update_obj_circle(Obj_circle arr[], int count, float dt)
{
    if(dt <= 0.0f) return;

    for(int i = 0; i < count; i++){
        // reset acceleration with base force  
        arr[i].body.acceleration.x = 0;
        arr[i].body.acceleration.y = 0;

        //calculate the implicit velocity(position delta)
        float vx = arr[i].body.position.x - arr[i].body.previous_position.x;
        float vy = arr[i].body.position.y - arr[i].body.previous_position.y;

        //store the current position;
        float tempX = arr[i].body.position.x;
        float tempY = arr[i].body.position.y;


        //calculate the next position
        arr[i].body.position.x = tempX + (tempX - arr[i].body.previous_position.x)
                                + (arr[i].body.acceleration.x * dt * dt);
        arr[i].body.position.y = tempY + (tempY - arr[i].body.previous_position.y)
                                + (arr[i].body.acceleration.y * dt * dt);

        //update the previous position with old current position
        arr[i].body.previous_position.x = tempX;
        arr[i].body.previous_position.y = tempY;

        //edge collision
        edge_collision_obj_circle(&arr[i]);
    }
}

void edge_collision_obj_circle(Obj_circle *obj)
{
    if(obj->body.position.x + obj->radius >= WIDTH){
        //find how fast it was moving inward
        float vx = obj->body.position.x - obj->body.previous_position.x;

        //clamp the position
        obj->body.position.x = WIDTH - obj->radius;

        //Trick the history! move the previous position forward
        // so next frame it thinks it was traveling backward
        obj->body.previous_position.x = obj->body.position.x + (vx * RESTITUTION);
    }
    if(obj->body.position.x - obj->radius <= 0){
        float vx = obj->body.position.x - obj->body.previous_position.x;
        obj->body.position.x = obj->radius;
        obj->body.previous_position.x = obj->body.position.x + (vx * RESTITUTION);
    }
    if(obj->body.position.y + obj->radius >= HEIGHT){
        float vy = obj->body.position.y - obj->body.previous_position.y;
        obj->body.position.y =  HEIGHT - obj->radius;
        obj->body.previous_position.y = obj->body.position.y + (vy * RESTITUTION);
    }
    if(obj->body.position.y - obj->radius <= 0){
        float vy = obj->body.position.y - obj->body.previous_position.y;
        obj->body.position.y =  obj->radius;
        obj->body.previous_position.y = obj->body.position.y + (vy * RESTITUTION);
    }
}


void elastic_collision_obj_circle(Obj_circle *a, Obj_circle *b, float dt)
{
    if(dt <= 0.0f) return;

    float dx = b->body.position.x - a->body.position.x;
    float dy = b->body.position.y - a->body.position.y;

    float dist = sqrt(dx*dx + dy*dy);
    float min_dist = a->radius + b->radius;

    if(dist < min_dist && dist > 0.0001f){
        
        float nx = dx/dist;
        float ny = dy/dist;

        float overlap = min_dist - dist;

        float inv_mass_a = 1.0f/a->body.mass;
        float inv_mass_b = 1.0f/b->body.mass;
        float total_inv_mass = inv_mass_a + inv_mass_b;

        float seperation_x = nx * (overlap / total_inv_mass);
        float seperation_y = ny * (overlap / total_inv_mass);

        a->body.position.x -= seperation_x * inv_mass_a;
        a->body.position.y -= seperation_y * inv_mass_a;
        b->body.position.x += seperation_x * inv_mass_b;
        b->body.position.y += seperation_y * inv_mass_b;

        float avx = (a->body.position.x - a->body.previous_position.x) / dt;
        float avy = (a->body.position.y - a->body.previous_position.y) / dt;
        float bvx = (b->body.position.x - b->body.previous_position.x) / dt;
        float bvy = (b->body.position.y - b->body.previous_position.y) / dt;

        float vx = bvx - avx;
        float vy = bvy - avy;

        float van = vx*nx + vy*ny;
        if(van >= 0.0f) return;
        
        float impulse = (-(1.0f + RESTITUTION) * van) / total_inv_mass;

        a->body.previous_position.x -= (impulse * inv_mass_a * nx) * dt;
        a->body.previous_position.y -= (impulse * inv_mass_a * ny) * dt;
        b->body.previous_position.x += (impulse * inv_mass_b * nx) * dt;
        b->body.previous_position.y += (impulse * inv_mass_b * ny) * dt;
    }
}




