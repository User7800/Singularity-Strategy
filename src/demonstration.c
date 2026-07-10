#include "raylib.h"
#include <flecs.h>
#include <stdio.h>

typedef struct {
    float x, y;
} Position, Velocity;

void Move(ecs_iter_t *it) {
  Position *p = ecs_field(it, Position, 0);
  Velocity *v = ecs_field(it, Velocity, 1);

  for (int i = 0; i < it->count; i++) {
    p[i].x += v[i].x;
    p[i].y += v[i].y;
  }
}

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight,  "Singularity Strategy - Now in pure C");
    SetTargetFPS(60);

    ecs_world_t *ecs = ecs_init();

    ECS_COMPONENT(ecs, Position);
    ECS_COMPONENT(ecs, Velocity);

    ECS_SYSTEM(ecs, Move, EcsOnUpdate, Position, Velocity);
    ecs_set_rate(ecs, ecs_id(Move), 1, 0);

    ecs_entity_t e = ecs_insert(ecs,
      ecs_value(Position, {10, 20}),
      ecs_value(Velocity, {0.5, 0}));

    const Position *p = ecs_get(ecs, e, Position);

    printf("=== Singularity Strategy Minimum Demo ===\n");
    printf("Controls: ESC to quit\n");

    while (!WindowShouldClose()) {
        ecs_progress(ecs, 0);
        BeginDrawing();
            ClearBackground(BLACK);
            DrawText("Singularity Strategy", 20, 20, 30, RAYWHITE);
            DrawText("Raylib + Flecs connected!", 20, 70, 20, GREEN);
            DrawText("A moving AI Core entity demonstrates ECS", 20, 110, 20, YELLOW);
            
            DrawCircle((int)p->x, (int)p->y + 200, 25, RED);
            DrawText("AI Core", (int)p->x - 35, (int)p->y + 150, 18, WHITE);
        EndDrawing();
    }

    CloseWindow();
    ecs_fini(ecs);
    return 0;
};