#include "raylib.h"
#include <flecs.h>
#include <iostream>

struct Position {
    float x = 0;
    float y = 0;
};

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Singularity Strategy - Minimum Demo");
    SetTargetFPS(60);

    flecs::world ecs;
    ecs.component<Position>();

    auto entity = ecs.entity("Test AI Core")
        .set(Position{400, 300});

    ecs.system<Position>()
        .each([](Position& p) {
            p.x += 0.5f;
            if (p.x > 900) p.x = 100;
        });

    std::cout << "=== Singularity Strategy Minimum Demo ===" << std::endl;
    std::cout << "Controls: ESC to quit" << std::endl;

    while (!WindowShouldClose()) {
        ecs.progress();

        BeginDrawing();
            ClearBackground(BLACK);
            DrawText("Singularity Strategy", 20, 20, 30, RAYWHITE);
            DrawText("Raylib + Flecs connected!", 20, 70, 20, GREEN);
            DrawText("A moving AI Core entity demonstrates ECS", 20, 110, 20, YELLOW);
            
            Position* pos = entity.get_mut<Position>();
            DrawCircle((int)pos->x, (int)pos->y + 200, 25, RED);
            DrawText("AI Core", (int)pos->x - 35, (int)pos->y + 150, 18, WHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
