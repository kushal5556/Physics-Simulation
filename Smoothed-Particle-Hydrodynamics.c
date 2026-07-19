#include "raylib.h"
#include <math.h>

#define WIDTH  800
#define HEIGHT 600

// ---- Global Variable ---
#define GRAVITY 50.0f
#define DAMPING 0.99f

#define PARTICLE_SIZE 25.0f
#define INFLUENCE_RADIUS 40.0f // neighbors

#define PARTICLE_COUNT 150

#define BASE_DENSITY 10.0f
#define MAX_DENSITY 255.0f

#define PRESSURE_STRENGTH 5.0f // compressibility 
#define VISCOSITY_STRENGTH 0.005f //thick or runny (liquidity)

//Gravity , Pressure Strength and Viscosity Strength must account for each other's value

#define MOUSE_RADIUS 50.0f
#define MOUSE_STRENGTH 30.0f

// --- Macros ----

// --- structs ----
typedef struct{
	Vector2 position;
	Vector2 velocity;
	Vector2 force;

	int neighborsIndex[PARTICLE_COUNT];
	size_t neighborCount;

	float density;
}Particle;

// --- function declarations ----
void init_particle(Particle particles[]);
void update_particle(Particle particles[], float dt);
void edge_collision(Particle particles[]);

int main()
{
	InitWindow(WIDTH, HEIGHT, "SPH water");
	SetTargetFPS(60);
	// --- variable declarations ---
	Particle particles[PARTICLE_COUNT];
	init_particle(particles);

	// --- game loop ----
	while(!WindowShouldClose()){
		// --- update everything ----
		float dt = GetFrameTime();

		if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
			Vector2 mp = GetMousePosition();
			for(int i = 0; i < PARTICLE_COUNT; i++){
				float dx = mp.x - particles[i].position.x; 
				float dy = mp.y - particles[i].position.y;

				float dist = sqrt(dx*dx + dy*dy);
				Vector2 mouse_rel = GetMouseDelta();
				if(dist <= MOUSE_RADIUS){
					if(dist == 0) dist = 0.01f;

					float nx = dx/dist;
					float ny = dy/dist;

					float gap = MOUSE_RADIUS - dist;
					// suck water toward mouse
					//particles[i].force.x += nx * gap * MOUSE_STRENGTH;	
					//particles[i].force.y += ny * gap * MOUSE_STRENGTH;	
					//push away 
					particles[i].force.x -= nx * gap * MOUSE_STRENGTH;	
					particles[i].force.y -= ny * gap * MOUSE_STRENGTH;	
				}
			}
		}

		if(IsKeyDown(KEY_W)){
			for(int i = 0; i < PARTICLE_COUNT; i++){
				particles[i].force.y -= 50;
			}
		}
		if(IsKeyDown(KEY_S)){
			for(int i = 0; i < PARTICLE_COUNT; i++){
				particles[i].force.y += 50;
			}
		}
		if(IsKeyDown(KEY_A)){
			for(int i = 0; i < PARTICLE_COUNT; i++){
				particles[i].force.x -= 50;
			}
		}
		if(IsKeyDown(KEY_D) ){
			for(int i = 0; i < PARTICLE_COUNT; i++){
				particles[i].force.x += 50;
			}
		}

		for(int i = 0; i < PARTICLE_COUNT; i++){
			particles[i].force.y += GRAVITY;

			particles[i].velocity.x += particles[i].force.x * dt;
			particles[i].velocity.y += particles[i].force.y * dt; 

			particles[i].position.x += particles[i].velocity.x * dt;
			particles[i].position.y += particles[i].velocity.y * dt;

			particles[i].velocity.x *= DAMPING;
			particles[i].velocity.y *= DAMPING;
		}

		update_particle(particles, dt);
		edge_collision(particles);

		// --- clear and draw ---
		BeginDrawing();
		ClearBackground(BLACK);

		for(int i = 0; i < PARTICLE_COUNT; i++){
			//  Debug: density (compressed) particles
			// if(particles[i].density < 40){
			// 	DrawCircleV(particles[i].position, PARTICLE_SIZE, BLUE);
			// }else if (particles[i].density < 60){
			// 	DrawCircleV(particles[i].position, PARTICLE_SIZE, YELLOW);
			// }else{
			// 	DrawCircleV(particles[i].position, PARTICLE_SIZE,  RED);
			// }
		 	DrawCircleV(particles[i].position, PARTICLE_SIZE, BLUE);
			//DrawCircleLines(particles[i].position.x, particles[i].position.y, INFLUENCE_RADIUS, BLUE);
		}

		EndDrawing();
	}
	// --- close everything ---
	CloseWindow();
	return 0;
}
// --- function definition ----
void init_particle(Particle particles[])
{
	for(int i = 0; i < PARTICLE_COUNT; i++){
		Particle new = {
			.position = (Vector2){GetRandomValue(PARTICLE_SIZE, WIDTH-PARTICLE_SIZE),
								GetRandomValue(PARTICLE_SIZE, HEIGHT-PARTICLE_SIZE)},
			.velocity = (Vector2){GetRandomValue(-15,15), GetRandomValue(-15,15)},
			.force = (Vector2){0.0f, 0.0f},
			.neighborsIndex = {0},
			.neighborCount = 0,
			.density =  BASE_DENSITY
		};
		particles[i] = new;
	}
}

void edge_collision(Particle particles[])
{
	for(int i = 0; i < PARTICLE_COUNT; i++)
	{
		float r = PARTICLE_SIZE;
		if(particles[i].position.x >= WIDTH-r){
			particles[i].position.x = WIDTH-r;
			particles[i].velocity.x *= -1;
		}
		if(particles[i].position.x <= r){
			particles[i].position.x = r;
			particles[i].velocity.x *= -1;
		}
		if(particles[i].position.y >= HEIGHT-r){
			particles[i].position.y = HEIGHT-r;
			particles[i].velocity.y *= -1;
		}
		if(particles[i].position.y <= r){
			particles[i].position.y = r;
			particles[i].velocity.y *= -1;
		}
	}
}
void update_particle(Particle particles[], float dt){
	//  calculate the density and find neighbors 
	for(int i = 0; i < PARTICLE_COUNT; i++){
		particles[i].neighborCount = 0;
		particles[i].density = BASE_DENSITY;

		for(int j = 0; j < PARTICLE_COUNT; j++){
			if(i == j) continue;

			if(particles[i].neighborCount >= PARTICLE_COUNT - 1) break;

			float dx = particles[j].position.x - particles[i].position.x;
			float dy = particles[j].position.y - particles[i].position.y;
			float dist = sqrt(dx*dx + dy*dy);

			if(dist < INFLUENCE_RADIUS){
				particles[i].neighborsIndex[particles[i].neighborCount++] = j;
				float gap = INFLUENCE_RADIUS - dist;
				if(gap > 0){
					particles[i].density += gap;
				}
			}
			if(particles[i].density > MAX_DENSITY){
				particles[i].density = MAX_DENSITY;
			}
		}
	}
	// calculate pressure force
	for(int i = 0; i < PARTICLE_COUNT; i++){
		particles[i].force.x = 0;
		particles[i].force.y = 0;

		float particle_crowd = particles[i].density - BASE_DENSITY;
		if(particle_crowd > 0){

			for(int j = 0; j < particles[i].neighborCount; j++){
				int neighborIdx = particles[i].neighborsIndex[j];

				float neighbor_crowd = particles[neighborIdx].density - BASE_DENSITY;

				if(particle_crowd < 0) particle_crowd = 0;
				if(neighbor_crowd < 0) neighbor_crowd = 0;

				// point from neighbor to particle
				float dx = particles[i].position.x - particles[neighborIdx].position.x;
				float dy = particles[i].position.y - particles[neighborIdx].position.y;

				float dist = sqrt(dx*dx + dy*dy);
				if(dist == 0) dist = 0.01;

				float nx = dx/dist;
				float ny = dy/dist;

				float gap = INFLUENCE_RADIUS - dist;
				float average_pressure = ((particle_crowd + neighbor_crowd)/2)/ PRESSURE_STRENGTH;

				float force_X = average_pressure * gap * nx;	
				float force_Y = average_pressure * gap * ny;	

				particles[i].force.x += force_X;
				particles[i].force.y += force_Y;
			}
		}	
	}
	//  viscosity (liquid friction)
	for(int i = 0; i < PARTICLE_COUNT; i++){
		for(int j = 0; j < particles[i].neighborCount; j++){
			int neighborIdx = particles[i].neighborsIndex[j];

			float rvx = particles[neighborIdx].velocity.x - particles[i].velocity.x;	
			float rvy = particles[neighborIdx].velocity.y - particles[i].velocity.y;	

			float dx = particles[i].position.x - particles[neighborIdx].position.x;
			float dy = particles[i].position.y - particles[neighborIdx].position.y;

			float dist = sqrt(dx*dx + dy*dy);

			float gap = INFLUENCE_RADIUS - dist;

			float viscosity_force_x = gap * rvx * VISCOSITY_STRENGTH;
			float viscosity_force_y = gap * rvy * VISCOSITY_STRENGTH;

			particles[i].force.x += viscosity_force_x;
			particles[i].force.y += viscosity_force_y;
		}
	}
}

