#include "raylib.h"
#include <math.h>
#include <stdio.h>

#define WIDTH  800
#define HEIGHT 600

// --- Global Variable ---
#define G 1
#define C 1

#define SOFTENING 0.001f

#define PHOTON_COUNT 100
// --- Macros ----
#define R_S(M) ((2.0f * G * M)/ (C * C))

// --- Structs ----
typedef struct{
	Vector2 position;
	float mass;  
	float r_s; 
}BlackHole;

typedef struct{
	Vector2 position;
	Vector2 velocity;
	float radius;
}Particle;

// --- Function Declarations ---
void update(Particle photons[], BlackHole *black_hole, float dt);

int main()
{
	InitWindow(WIDTH, HEIGHT, "Black Hole Ray Tracer");
	SetTargetFPS(60);
	// --- Variable Declarations ----
	BlackHole black_hole = {
		.position = (Vector2){400, 300},
		.mass = 50,
		.r_s = R_S(50) //mass*2
	};

	Particle photons[PHOTON_COUNT];
	Particle photonsOrigin[PHOTON_COUNT];
	for(int i = 0; i < PHOTON_COUNT; i++){
		Particle new = {
			.position = (Vector2){5, GetRandomValue(5,595)},
			.velocity = (Vector2){50,0},
			.radius = 5.0f
		};
		photons[i] = new;
		photonsOrigin[i] = new;
	}

	// --- game loop ---
	while(!WindowShouldClose()){
		// --- update ---
		float dt = GetFrameTime();

		update(photons, &black_hole, dt);

		// --- clear and draw ---
		BeginDrawing();
		ClearBackground(BLACK);

		for(int i = 0; i < PHOTON_COUNT; i++){
			DrawCircleV(photons[i].position, photons[i].radius, RAYWHITE);
			DrawLineV(photonsOrigin[i].position, photons[i].position, RAYWHITE);
		}
		DrawCircleLinesV(black_hole.position, black_hole.r_s, WHITE);

		EndDrawing();
	}
	// --- close ---
	CloseWindow();

	return 0;
}

void update(Particle photons[], BlackHole *black_hole, float dt)
	{
		for(int i = 0; i < PHOTON_COUNT; i++){
			photons[i].position.x += photons[i].velocity.x * dt;
			photons[i].position.y += photons[i].velocity.y * dt;

			float dx = black_hole->position.x - photons[i].position.x;
			float dy = black_hole->position.y - photons[i].position.y;

			float dist_sq = (dx*dx + dy*dy) + SOFTENING;
			float dist = sqrtf(dist_sq);

			if(dist <= black_hole->r_s + photons[i].radius){
				photons[i].velocity.x = 0;
				photons[i].velocity.y = 0;

			}else if(dist <= (black_hole->r_s*4)){
				float nx = dx/dist;
				float ny = dy/dist;

				float gravity = 200000/dist_sq;  // closer dist = stronger gravity

				photons[i].velocity.x += nx * gravity * dt;
				photons[i].velocity.y += ny * gravity * dt;
			}
		}
	}