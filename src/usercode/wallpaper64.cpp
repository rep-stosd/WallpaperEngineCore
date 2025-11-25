#define IMGUI_DEFINE_MATH_OPERATORS
#include "CScene.hpp"
#include "R_Common.hpp"
#include "imgui/imgui_internal.h"

Scene scene;

REND_StateManager stateManager;
/*
 DEVELOPMENT GOALS:
 - add in-memory streaming of mp4s (MaterialComponent.cpp:VideoComponent)
 - reimplement video/picture loading (CScene.cpp)
 - add the render pipeline
  - implement hlsl shader reading (R_ShaderParser.cpp)
 
- re-port back to ios & add appropriate springboard hooks
*/

void NetGameImguiWindow() {
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(4,2), IM_COL32_WHITE, "WORK IN PROGRESS");
    static bool _testInit = false;
    if (!_testInit) {
        stateManager.init();
        _testInit = true;
    }
    void Wallpaper64SteamUI();
    Wallpaper64SteamUI();
    
}
