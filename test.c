#include "raylib.h"

int main(void) {
  const int screenWidth = 800;
  const int screenHeight = 450;
  const int fontSize = 40;
  const char *message = "Hello, World!";

  InitWindow(screenWidth, screenHeight, "Hello, World! - raylib");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText(message,
             (GetScreenWidth() - MeasureText(message, fontSize)) / 2,
             (GetScreenHeight() - fontSize) / 2,
             fontSize, DARKGRAY);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
