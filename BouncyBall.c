#include <stdio.h>
#include "raylib.h"

#define WIDTH 800
#define HEIGHT 600

int main()
{
    InitWindow(WIDTH, HEIGHT, "Bouncy Ball");
    SetTargetFPS(60);

    while(!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(BLACK);

        EndDrawing();
    }
    CloseWindow();
    return 0;
}    

