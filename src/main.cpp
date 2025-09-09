#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <string>
#include <vector>

enum class GameState { Menu, CharacterSelect, Arena, Exit };

struct CharacterDef {
    std::string name;
    std::string gltfPath;
    std::string textureDir;
};

struct LoadedCharacter {
    CharacterDef def;
    Model model;
    Texture2D texture;
    bool loaded = false;
};

static std::vector<CharacterDef> kCharacters = {
    {"Asgore", "models/asgore/scene.gltf", "models/asgore/textures"},
    {"Metrocop", "models/metrocop/scene.gltf", "models/metrocop/textures"}
};

static int ClampIndex(int value, int minValue, int maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "epiCBattle");
    SetTargetFPS(60);

    GameState gameState = GameState::Menu;

    // Camera for 3D scenes
    Camera3D camera = { 0 };
    camera.position = { 6.0f, 4.0f, 6.0f };
    camera.target = { 0.0f, 1.5f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    int selectedIndex = 0;
    std::vector<LoadedCharacter> loaded(kCharacters.size());

    auto unloadAll = [&]() {
        for (auto &lc : loaded) {
            if (lc.loaded) {
                UnloadTexture(lc.texture);
                UnloadModel(lc.model);
                lc.loaded = false;
            }
        }
    };

    auto ensureLoaded = [&](int index) {
        if (index < 0 || index >= (int)kCharacters.size()) return;
        if (loaded[index].loaded) return;
        const CharacterDef &def = kCharacters[index];
        LoadedCharacter lc;
        lc.def = def;
        // raylib loads .gltf via tinygltf; textures embedded or external. We'll load model only.
        lc.model = LoadModel(def.gltfPath.c_str());
        if (lc.model.materialCount > 0) {
            // Try to assign base color texture if found; otherwise leave as-is
            // We look for any png in textureDir ending with _baseColor or first png
            // Minimal: try known names
            std::string candidate;
            if (def.name == "Asgore") candidate = def.textureDir + "/Asgore_Mat_baseColor.png";
            else if (def.name == "Metrocop") candidate = def.textureDir + "/metrocop_body_baseColor.png";
            Image img = LoadImage(candidate.c_str());
            if (img.data) {
                lc.texture = LoadTextureFromImage(img);
                UnloadImage(img);
                for (int m = 0; m < lc.model.materialCount; ++m) {
                    SetMaterialTexture(&lc.model.materials[m], MATERIAL_MAP_ALBEDO, lc.texture);
                }
            } else {
                // Create a white texture placeholder
                Image white = GenImageColor(2, 2, RAYWHITE);
                lc.texture = LoadTextureFromImage(white);
                UnloadImage(white);
            }
        }
        loaded[index] = lc;
        loaded[index].loaded = true;
    };

    ensureLoaded(selectedIndex);

    // Simple ground
    Vector3 arenaSize = { 20.0f, 1.0f, 20.0f };

    while (!WindowShouldClose()) {
        // Update
        switch (gameState) {
            case GameState::Menu: {
                if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    gameState = GameState::CharacterSelect;
                }
            } break;
            case GameState::CharacterSelect: {
                int delta = 0;
                if (IsKeyPressed(KEY_RIGHT) || GetMouseWheelMove() < 0) delta = 1;
                if (IsKeyPressed(KEY_LEFT) || GetMouseWheelMove() > 0) delta = -1;
                if (delta != 0) {
                    selectedIndex = ClampIndex(selectedIndex + delta, 0, (int)kCharacters.size() - 1);
                    ensureLoaded(selectedIndex);
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    gameState = GameState::Arena;
                }
                if (IsKeyPressed(KEY_ESCAPE)) {
                    gameState = GameState::Menu;
                }
            } break;
            case GameState::Arena: {
                // Basic orbit camera controls
                UpdateCamera(&camera, CAMERA_ORBITAL);
                if (IsKeyPressed(KEY_ESCAPE)) {
                    gameState = GameState::CharacterSelect;
                }
            } break;
            case GameState::Exit: {
                CloseWindow();
                return 0;
            }
        }

        // Draw
        BeginDrawing();
        ClearBackground(BLACK);

        if (gameState == GameState::Menu) {
            DrawText("epiCBattle", 40, 40, 64, RAYWHITE);
            DrawText("Press Enter to Start", 40, 130, 30, LIGHTGRAY);
        } else if (gameState == GameState::CharacterSelect) {
            DrawText("Select Your Fighter", 40, 40, 48, RAYWHITE);
            DrawText("Left/Right to change, Enter to confirm, Esc to back", 40, 100, 20, GRAY);
            const int margin = 40;
            int x = margin;
            int y = 160;
            for (int i = 0; i < (int)kCharacters.size(); ++i) {
                Color color = (i == selectedIndex) ? YELLOW : GRAY;
                DrawRectangleLines(x - 10, y - 10, 280, 60, color);
                DrawText(kCharacters[i].name.c_str(), x, y, 40, color);
                y += 80;
            }
            // Preview 3D model on the right
            int vpX = GetScreenWidth() / 2;
            int vpW = GetScreenWidth() / 2 - margin;
            int vpY = 140;
            int vpH = GetScreenHeight() - vpY - margin;
            Rectangle vp = { (float)vpX, (float)vpY, (float)vpW, (float)vpH };
            rlViewport(vp.x, vp.y, vp.width, vp.height);
            BeginMode3D(camera);
            DrawGrid(10, 1.0f);
            if (selectedIndex >= 0 && selectedIndex < (int)loaded.size() && loaded[selectedIndex].loaded) {
                DrawModel(loaded[selectedIndex].model, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
            }
            EndMode3D();
            rlViewport(0, 0, GetScreenWidth(), GetScreenHeight());
            DrawRectangleLines(vpX, vpY, vpW, vpH, DARKGRAY);
        } else if (gameState == GameState::Arena) {
            BeginMode3D(camera);
            DrawPlane({0.0f, 0.0f, 0.0f}, {arenaSize.x, arenaSize.z}, DARKGREEN);
            DrawCube({-arenaSize.x * 0.5f, 0.5f, 0.0f}, 1.0f, 1.0f, 1.0f, RED);
            DrawCube({ arenaSize.x * 0.5f, 0.5f, 0.0f}, 1.0f, 1.0f, 1.0f, BLUE);
            if (selectedIndex >= 0 && selectedIndex < (int)loaded.size() && loaded[selectedIndex].loaded) {
                DrawModel(loaded[selectedIndex].model, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
            }
            EndMode3D();
            DrawText("Esc: Back", 20, 20, 20, GRAY);
        }

        EndDrawing();
    }

    unloadAll();
    CloseWindow();
    return 0;
}


