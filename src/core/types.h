#pragma once

#include "raylib.h"
#include <vector>

enum class GameState { Menu, ModeSelect, MapSelect, CharacterSelect, Settings, Arena, Pause, Exit };
enum class ViewMode { FirstPerson, ThirdPerson };
enum class MapType { Green, Desert };

struct AABB { Vector3 min; Vector3 max; };


