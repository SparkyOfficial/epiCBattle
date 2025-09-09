#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <string>
#include <vector>
#include <cmath>

enum class GameState { Menu, CharacterSelect, Settings, Arena, Exit };
enum class ViewMode { FirstPerson, ThirdPerson };

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
    int screenWidth = 1600;
    int screenHeight = 900;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "epiCBattle");
    SetTargetFPS(120);

    GameState gameState = GameState::Menu;

    // Settings
    float mouseSensitivity = 0.25f;
    float fieldOfView = 70.0f;
    bool lockCursor = true;

    // Camera for 3D scenes
    Camera3D camera = { 0 };
    camera.position = { 0.0f, 1.7f, 4.0f };
    camera.target = { 0.0f, 1.7f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = fieldOfView;
    camera.projection = CAMERA_PERSPECTIVE;
    ViewMode viewMode = ViewMode::FirstPerson;

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
    Vector3 arenaSize = { 30.0f, 1.0f, 30.0f };

    // Simple "server" state for local multiplayer (2 players)
    struct PlayerState {
        Vector3 position;
        float velocityY;
        float yawRadians;
        int health;
        bool attacking;
        float attackCooldown;
        float attackTimer;
        int characterIndex;
    };
    struct ServerState {
        PlayerState players[2];
    } server;
    auto resetPlayer = [&](PlayerState &p){
        p.position = {0.0f, 0.0f, 0.0f};
        p.velocityY = 0.0f;
        p.yawRadians = 0.0f;
        p.health = 100;
        p.attacking = false;
        p.attackCooldown = 0.0f;
        p.attackTimer = 0.0f;
        p.characterIndex = selectedIndex;
    };
    resetPlayer(server.players[0]);
    server.players[0].position = {-4.0f, 0.0f, 0.0f};
    resetPlayer(server.players[1]);
    server.players[1].position = { 4.0f, 0.0f, 0.0f};

    auto damageIfInRange = [&](int attacker, int victim){
        const float range = 2.5f;
        const int damage = 10;
        Vector3 a = server.players[attacker].position;
        Vector3 b = server.players[victim].position;
        float d = Vector3Distance(a, b);
        if (d <= range) {
            server.players[victim].health -= damage;
            if (server.players[victim].health < 0) server.players[victim].health = 0;
        }
    };

    float fixedDt = 1.0f/60.0f;
    double accumulator = 0.0;
    double lastTime = GetTime();

    while (!WindowShouldClose()) {
        // Update
        if (IsKeyPressed(KEY_F11)) {
            bool fs = IsWindowFullscreen();
            if (!fs) {
                int monitor = GetCurrentMonitor();
                int mw = GetMonitorWidth(monitor);
                int mh = GetMonitorHeight(monitor);
                ToggleFullscreen();
                SetWindowSize(mw, mh);
            } else {
                ToggleFullscreen();
                SetWindowSize(screenWidth, screenHeight);
            }
        }
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
                if (IsKeyPressed(KEY_S)) {
                    gameState = GameState::Settings;
                }
            } break;
            case GameState::Settings: {
                // Adjust FOV and sensitivity
                if (IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_EQUAL)) fieldOfView = Clamp(fieldOfView + 2.0f, 50.0f, 110.0f);
                if (IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_MINUS)) fieldOfView = Clamp(fieldOfView - 2.0f, 50.0f, 110.0f);
                if (IsKeyPressed(KEY_RIGHT_BRACKET)) mouseSensitivity = Clamp(mouseSensitivity + 0.02f, 0.05f, 1.0f);
                if (IsKeyPressed(KEY_LEFT_BRACKET)) mouseSensitivity = Clamp(mouseSensitivity - 0.02f, 0.05f, 1.0f);
                camera.fovy = fieldOfView;
                if (IsKeyPressed(KEY_L)) lockCursor = !lockCursor;
                if (IsKeyPressed(KEY_ESCAPE)) gameState = GameState::Menu;
                if (IsKeyPressed(KEY_ENTER)) gameState = GameState::CharacterSelect;
            } break;
            case GameState::Arena: {
                // Switch view mode
                if (IsKeyPressed(KEY_C)) {
                    viewMode = (viewMode == ViewMode::FirstPerson) ? ViewMode::ThirdPerson : ViewMode::FirstPerson;
                }
                if (IsKeyPressed(KEY_P)) {
                    gameState = GameState::Settings;
                }

                // Input for two local players
                auto moveDir = [&](int playerIndex){
                    Vector3 dir = {0};
                    if (playerIndex == 0) {
                        if (IsKeyDown(KEY_W)) dir.z -= 1.0f;
                        if (IsKeyDown(KEY_S)) dir.z += 1.0f;
                        if (IsKeyDown(KEY_A)) dir.x -= 1.0f;
                        if (IsKeyDown(KEY_D)) dir.x += 1.0f;
                    } else {
                        if (IsKeyDown(KEY_UP)) dir.z -= 1.0f;
                        if (IsKeyDown(KEY_DOWN)) dir.z += 1.0f;
                        if (IsKeyDown(KEY_LEFT)) dir.x -= 1.0f;
                        if (IsKeyDown(KEY_RIGHT)) dir.x += 1.0f;
                    }
                    return dir;
                };

                auto doAttackPressed = [&](int playerIndex){
                    return playerIndex == 0 ? IsMouseButtonPressed(MOUSE_BUTTON_LEFT) : IsKeyPressed(KEY_RIGHT_CONTROL);
                };

                // Fixed-step server tick
                double now = GetTime();
                accumulator += (now - lastTime);
                lastTime = now;
                while (accumulator >= fixedDt) {
                    for (int i = 0; i < 2; ++i) {
                        Vector3 dir = moveDir(i);
                        if (Vector3Length(dir) > 0.0f) dir = Vector3Normalize(dir);
                        float speed = 5.0f;
                        if ((i == 0 && IsKeyDown(KEY_LEFT_SHIFT)) || (i == 1 && IsKeyDown(KEY_RIGHT_SHIFT))) speed *= 1.8f;
                        // Rotate to movement direction if any
                        if (dir.x != 0.0f || dir.z != 0.0f) {
                            server.players[i].yawRadians = atan2f(-dir.x, -dir.z);
                        }
                        // Move in world plane XZ
                        server.players[i].position.x += dir.x * speed * fixedDt;
                        server.players[i].position.z += dir.z * speed * fixedDt;

                        // Jump/gravity
                        const float gravity = -22.0f;
                        bool wantJump = (i == 0) ? IsKeyPressed(KEY_SPACE) : IsKeyPressed(KEY_RIGHT_SHIFT);
                        bool grounded = (server.players[i].position.y <= 0.0f);
                        if (grounded) {
                            server.players[i].position.y = 0.0f;
                            if (wantJump) server.players[i].velocityY = 8.5f;
                        }
                        server.players[i].velocityY += gravity * fixedDt;
                        server.players[i].position.y += server.players[i].velocityY * fixedDt;
                        if (server.players[i].position.y < 0.0f) {
                            server.players[i].position.y = 0.0f;
                            server.players[i].velocityY = 0.0f;
                        }

                        // Clamp to arena bounds
                        server.players[i].position.x = Clamp(server.players[i].position.x, -arenaSize.x * 0.5f + 1.0f, arenaSize.x * 0.5f - 1.0f);
                        server.players[i].position.z = Clamp(server.players[i].position.z, -arenaSize.z * 0.5f + 1.0f, arenaSize.z * 0.5f - 1.0f);

                        // Attack logic
                        server.players[i].attackCooldown -= fixedDt;
                        if (server.players[i].attackCooldown < 0.0f) server.players[i].attackCooldown = 0.0f;
                        if (doAttackPressed(i) && server.players[i].attackCooldown <= 0.0f) {
                            server.players[i].attacking = true;
                            server.players[i].attackTimer = 0.2f;
                            server.players[i].attackCooldown = 0.6f;
                            int victim = (i == 0) ? 1 : 0;
                            damageIfInRange(i, victim);
                        }
                        if (server.players[i].attacking) {
                            server.players[i].attackTimer -= fixedDt;
                            if (server.players[i].attackTimer <= 0.0f) server.players[i].attacking = false;
                        }
                    }
                    accumulator -= fixedDt;
                }

                // Camera update based on primary player (index 0)
                const PlayerState &p0 = server.players[0];
                if (viewMode == ViewMode::FirstPerson) {
                    // Mouse look
                    if (lockCursor) DisableCursor(); else EnableCursor();
                    Vector2 md = GetMouseDelta();
                    static float pitch = 0.0f;
                    p0; // silence unused warning for reference
                    server.players[0].yawRadians -= md.x * 0.01f * mouseSensitivity;
                    pitch -= md.y * 0.01f * mouseSensitivity;
                    pitch = Clamp(pitch, -1.3f, 1.3f);
                    Vector3 forward = { -sinf(server.players[0].yawRadians), 0.0f, -cosf(server.players[0].yawRadians) };
                    Vector3 right = { -forward.z, 0.0f, forward.x };
                    // Move with WASD relative to look
                    Vector3 rel = {0};
                    if (IsKeyDown(KEY_W)) rel = Vector3Add(rel, forward);
                    if (IsKeyDown(KEY_S)) rel = Vector3Subtract(rel, forward);
                    if (IsKeyDown(KEY_A)) rel = Vector3Subtract(rel, right);
                    if (IsKeyDown(KEY_D)) rel = Vector3Add(rel, right);
                    if (Vector3Length(rel) > 0) rel = Vector3Normalize(rel);
                    float speed = 6.0f * GetFrameTime();
                    if (IsKeyDown(KEY_LEFT_SHIFT)) speed *= 1.8f;
                    server.players[0].position = Vector3Add(server.players[0].position, Vector3Scale(rel, speed));
                    camera.position = Vector3Add(server.players[0].position, {0.0f, 1.7f, 0.0f});
                    Vector3 lookDir = { cosf(pitch) * -sinf(server.players[0].yawRadians), sinf(pitch), cosf(pitch) * -cosf(server.players[0].yawRadians) };
                    camera.target = Vector3Add(camera.position, lookDir);
                } else {
                    // Third-person camera: orbit behind player 0
                    float dist = 5.0f;
                    float height = 2.0f;
                    Vector3 back = { sinf(server.players[0].yawRadians), 0.0f, cosf(server.players[0].yawRadians) };
                    camera.target = Vector3Add(server.players[0].position, {0.0f, 1.5f, 0.0f});
                    camera.position = Vector3Add(camera.target, Vector3Add(Vector3Scale(back, dist), Vector3{0.0f, height, 0.0f}));
                    EnableCursor();
                }

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
            DrawText("Press S for Settings", 40, 170, 24, GRAY);
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
            rlViewport((int)vp.x, (int)vp.y, (int)vp.width, (int)vp.height);
            BeginMode3D(camera);
            DrawGrid(10, 1.0f);
            if (selectedIndex >= 0 && selectedIndex < (int)loaded.size() && loaded[selectedIndex].loaded) {
                // Idle sway
                float t = (float)GetTime();
                float scale = 1.0f + 0.03f * sinf(t * 2.0f);
                Vector3 pos = {0.0f, 0.0f, 0.0f};
                DrawModelEx(loaded[selectedIndex].model, pos, {0,1,0}, 0.0f, {scale, scale, scale}, WHITE);
            }
            EndMode3D();
            rlViewport(0, 0, GetScreenWidth(), GetScreenHeight());
            DrawRectangleLines(vpX, vpY, vpW, vpH, DARKGRAY);
        } else if (gameState == GameState::Arena) {
            BeginMode3D(camera);
            DrawPlane({0.0f, 0.0f, 0.0f}, {arenaSize.x, arenaSize.z}, DARKGREEN);
            DrawCube({-arenaSize.x * 0.5f, 0.5f, 0.0f}, 1.0f, 1.0f, 1.0f, RED);
            DrawCube({ arenaSize.x * 0.5f, 0.5f, 0.0f}, 1.0f, 1.0f, 1.0f, BLUE);
            // Draw two players
            for (int i = 0; i < 2; ++i) {
                int ci = server.players[i].characterIndex;
                if (ci >= 0 && ci < (int)loaded.size() && loaded[ci].loaded) {
                    float t = (float)GetTime();
                    float moveSway = 0.02f * sinf(t * 6.0f);
                    float atkPulse = server.players[i].attacking ? 0.2f : 0.0f;
                    float scale = 1.0f + moveSway + atkPulse;
                    Color tint = i == 0 ? WHITE : LIGHTGRAY;
                    DrawModelEx(
                        loaded[ci].model,
                        server.players[i].position,
                        {0,1,0},
                        server.players[i].yawRadians * RAD2DEG,
                        {scale, scale, scale},
                        tint
                    );
                }
            }
            EndMode3D();
            // HUD
            DrawText("Esc: Back | C: View | P: Settings | F11: Fullscreen", 20, 20, 20, GRAY);
            // Health bars
            float barW = 300.0f;
            float barH = 20.0f;
            DrawRectangle(20, 60, (int)barW, (int)barH, DARKGRAY);
            DrawRectangle(20, 60, (int)(barW * (server.players[0].health/100.0f)), (int)barH, RED);
            DrawRectangle(GetScreenWidth() - 20 - (int)barW, 60, (int)barW, (int)barH, DARKGRAY);
            DrawRectangle(GetScreenWidth() - 20 - (int)barW, 60, (int)(barW * (server.players[1].health/100.0f)), (int)barH, BLUE);
            // Crosshair for FPS
            if (viewMode == ViewMode::FirstPerson) {
                int cx = GetScreenWidth()/2;
                int cy = GetScreenHeight()/2;
                DrawLine(cx-8, cy, cx+8, cy, RAYWHITE);
                DrawLine(cx, cy-8, cx, cy+8, RAYWHITE);
            }
        } else if (gameState == GameState::Settings) {
            DrawText("Settings", 40, 40, 48, RAYWHITE);
            DrawText(TextFormat("FOV: %d  (+/= , -)", (int)fieldOfView), 40, 110, 24, LIGHTGRAY);
            DrawText(TextFormat("Mouse sensitivity: %.2f  ([ , ])", mouseSensitivity), 40, 140, 24, LIGHTGRAY);
            DrawText(TextFormat("Cursor lock (L): %s", lockCursor ? "ON" : "OFF"), 40, 170, 24, LIGHTGRAY);
            DrawText("Enter: Back to Select | Esc: Main Menu", 40, 210, 20, GRAY);
        }

        EndDrawing();
    }

    unloadAll();
    CloseWindow();
    return 0;
}


