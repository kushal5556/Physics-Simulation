#include <math.h>
#include <stdbool.h>
#include "raylib.h"

#define WIDTH 800
#define HEIGHT 600

// --------- Global Variable --------------
#define GRAVITY 9.8
#define RESTITUTION 0.8

// -------- Structs -----------
typedef struct {
    Vector2 current_position;
    Vector2 previous_position;
    Vector2 acceleration;
}Object;

typedef struct{
    Object body;
    float radius;
    Color color;
}Circle;


int main()
{
    InitWindow(WIDTH, HEIGHT, "Bouncy Ball");
    SetTargetFPS(60);

    // ---------------- variable declarations ----------------
    Circle a = {
        .body = (Object){
            .current_position  = (Vector2){400, 300},
            .previous_position = (Vector2){400, 300},
            .acceleration = (Vector2){0, 0}
        },
        .radius = 50,
        .color = WHITE
    };

    // ------------- game loop -------------------
    while(!WindowShouldClose()){
        // --------------- update everything --------------------
        float dt = GetFrameTime();

        //acceleration reset with base forces
        a.body.acceleration.x = 0;
        a.body.acceleration.y = GRAVITY;

        //acceleration oppose velocity
        // calculate the implicit velocity
        float vx = a.body.current_position.x - a.body.previous_position.x;
        float vy = a.body.current_position.y - a.body.previous_position.y;

        float speed = sqrt(vx*vx + vy*vy) / dt; // speed per second
        float dragCoefficient = 0.005; //resistance from environment(smaller->less(faster))
        float dragMagnitude = dragCoefficient * speed * speed; // speed squared

        // Apply it in the opposite direction of velocity
        if (speed > 0) {
            // vx/dt to turn the raw position deltat back into a per-second velocity
            a.body.acceleration.x -= ((vx / dt) / speed) * dragMagnitude;
            a.body.acceleration.y -= ((vy / dt) / speed) * dragMagnitude;
        }

        //player input update acceleration 
        if(IsKeyDown(KEY_W)) a.body.acceleration.y  -= 50.0;
        if(IsKeyDown(KEY_S)) a.body.acceleration.y  += 50.0;
        if(IsKeyDown(KEY_A)) a.body.acceleration.x  -= 50.0;
        if(IsKeyDown(KEY_D)) a.body.acceleration.x  += 50.0;

        //store the current position
        float tempX = a.body.current_position.x;
        float tempY = a.body.current_position.y;

        // calculate the next position using current, previous and acceleration
        a.body.current_position.x = tempX + (tempX - a.body.previous_position.x) + a.body.acceleration.x * dt * dt;
        a.body.current_position.y = tempY + (tempY - a.body.previous_position.y) + a.body.acceleration.y * dt * dt;

        //update previous position to be old current position
        a.body.previous_position.x = tempX;
        a.body.previous_position.y = tempY;
        
        // check edge collision 
        if( a.body.current_position.x + a.radius >= WIDTH){
            //find how fast it was moving inward
            float vx = a.body.current_position.x - a.body.previous_position.x;
            a.body.current_position.x = WIDTH - a.radius;
            //trick the history! move the prvious position forward
            // so next frame it thinks it was traveling backward
            a.body.previous_position.x = a.body.current_position.x + (vx * RESTITUTION);
        } 
        if( a.body.current_position.x - a.radius <= 0){
            float vx = a.body.current_position.x - a.body.previous_position.x; 
            a.body.current_position.x = a.radius;

            a.body.previous_position.x = a.body.current_position.x + (vx * RESTITUTION);
        }
        if( a.body.current_position.y + a.radius >= HEIGHT){ 
            float vy = a.body.current_position.y - a.body.previous_position.y;
            a.body.current_position.y = HEIGHT - a.radius;
            // if it is in very slow speed, stop bouncing 
            if(abs(vy/dt) < 0.5){
                a.body.previous_position.y = a.body.current_position.y;// lock it to the floor
                a.body.acceleration.y = 0; // cancel gravity while resting
            }else{
                a.body.previous_position.y = a.body.current_position.y + (vy * RESTITUTION);
            }
        }
        if( a.body.current_position.y - a.radius <= 0){
            float vy = a.body.current_position.y - a.body.previous_position.y;
            a.body.current_position.y = a.radius;
            a.body.previous_position.y = a.body.current_position.y + (vy * RESTITUTION);
        }


        // ----------- clear and draw --------------------
        BeginDrawing();
        ClearBackground(BLACK);

        DrawCircleV(a.body.current_position, a.radius, a.color);

        EndDrawing();
    }
    // ----------- close everything ----------------
    CloseWindow();
    return 0;
}    

