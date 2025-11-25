#define IMGUI_DEFINE_MATH_OPERATORS
#include "CScene.hpp"
#include "imgui/imgui_internal.h"

Scene scene;

/*
 DEVELOPMENT GOALS:
 - add in-memory streaming of mp4s (MaterialComponent.cpp:VideoComponent)
 - add the render pipeline
  - implement hlsl shader reading (R_ShaderParser.cpp)
 
- re-port back to ios & add appropriate springboard hooks
*/

void NetGameImguiWindow() {
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(4,2), IM_COL32_WHITE, "WORK IN PROGRESS");
    
    void Wallpaper64SteamUI();
    Wallpaper64SteamUI();
    
}
