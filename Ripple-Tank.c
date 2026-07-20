#include "raylib.h"
#include <math.h>

#define WIDTH  800
#define HEIGHT 600

// --- Global Variable ----
#define BACKGROUND_COLOR (Color)SKYBLUE

#define MAX_WAVE 30

#define WAVE_SPEED 60.0f
#define WAVE_SPACING 0.4f // second

#define BASE_AMPLITUDE 8.0f
#define DECAY_RATE 0.02f

// --- structs ---
typedef struct{
    Vector2 position; 
    float radius;
    float amplitude;
    Color color;
}My_Wave;


// --- function declaration ---

int main()
{
	InitWindow(WIDTH, HEIGHT, "Ripple Tank Simulation");
	SetTargetFPS(60);
	// --- Variable Declarations----
    My_Wave waves[MAX_WAVE];
    size_t current_wave = 0;
    size_t wave_index = 0;

    Vector2 source = {400, 300};

    float time_count = 0.0f;
	// ---game loop ----
	while(!WindowShouldClose()){
		//---update----	
		float dt = GetFrameTime();
        time_count += dt;

        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
            source = GetMousePosition();
        }
        if(IsKeyDown(KEY_W)) source.y -= WAVE_SPEED * dt;
        if(IsKeyDown(KEY_S)) source.y += WAVE_SPEED * dt;
        if(IsKeyDown(KEY_D)) source.x += WAVE_SPEED * dt;
        if(IsKeyDown(KEY_A)) source.x -= WAVE_SPEED * dt;

        if(time_count > WAVE_SPACING){
            time_count -= WAVE_SPACING;

            static int isBlack = 0;
            My_Wave new = {
                .position = source,
                .radius = 0,
                .amplitude = BASE_AMPLITUDE,
                .color = BLACK
            };
            switch (isBlack){
                case 0: 
                    new.color = BLACK;
                    isBlack++;
                    break;
                case 1: 
                    new.color = WHITE;
                    isBlack++;
                    break;
                case 2: 
                    new.color = GREEN;
                    isBlack++;
                    break;
                case 3: 
                    new.color = BLUE;
                    isBlack++;
                    break;
                case 4: 
                    new.color = YELLOW;
                    isBlack++;
                    break;
                case 5: 
                    new.color = PINK;
                    isBlack++;
                    break;
                case 6: 
                    new.color = RED;
                    isBlack = 0;
                    break;
                default: 
                    break;
            }
            
            waves[wave_index % MAX_WAVE] = new;

            wave_index++;
            if(wave_index >= MAX_WAVE)  wave_index = 0;
            if(current_wave < MAX_WAVE) current_wave++;
        }

        for(int i = 0; i < current_wave; i++){
            waves[i].radius += WAVE_SPEED * dt;
            if(waves[i].color.a > 50)
                waves[i].color.a -= 0.01f;
            if(waves[i].amplitude > 1)
                waves[i].amplitude -= DECAY_RATE;
        }


		//----clear and draw----
		BeginDrawing();
		ClearBackground(BACKGROUND_COLOR);

        DrawRectangleV((Vector2){source.x-4, source.y-4}, (Vector2){8,8}, BLACK);
        for(int i = 0; i < current_wave; i++){
            DrawRing(waves[i].position, waves[i].radius, waves[i].radius+waves[i].amplitude, 0.0f, 360.0f, 36, waves[i].color);
        }

		EndDrawing();
	}
	//---- close ----
	CloseWindow();
	return 0;
} 
//------ Function Definition -----

