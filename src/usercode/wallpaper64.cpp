#define IMGUI_DEFINE_MATH_OPERATORS
#include "CScene.hpp"
#include "R_Common.hpp"
#include "imgui/imgui_internal.h"

Scene scene;
REND_CommandList cmdList;
/*
 DEVELOPMENT GOALS:
 - add in-memory streaming of mp4s (MaterialComponent.cpp:VideoComponent)
 - reimplement video/picture loading (CScene.cpp)
 - add the render pipeline
  - implement hlsl shader reading (R_ShaderParser.cpp)
 
- re-port back to ios & add appropriate springboard hooks
*/

void R_SetupConstantBuffers()
{
    
}

void R_BuildFrameData()
{
    
}

void NetGameImguiWindow() {
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(5,3), IM_COL32_BLACK, "WORK IN PROGRESS");
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(4,2), IM_COL32_WHITE, "WORK IN PROGRESS");
    
    void Wallpaper64SteamUI();
    Wallpaper64SteamUI();
    
    static bool _native = false;
    
    ImGui::Begin("Test");
    ImGui::DragFloat2("offset", &m_stateManager.off_x);
    ImGui::DragFloat2("sz", &m_stateManager.sz_x);
    ImGui::DragInt("cnt", &m_stateManager.cnt);
    ImGui::Checkbox("native render", &_native);
    ImGui::End();
    
    
    
    if (_native)
        return;
    
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::Begin("canva", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing);
    
    scene.update();
    {
        
        float _size = m_stateManager.sz_x;
        float _scale = m_stateManager.sz_y;
        
        int c = 0;
        for (auto* i : scene.nodes) {
            if (c > m_stateManager.cnt) break;
            if (i->layer && i->layer->material.texture) {
                if (ImageLayer* r = dynamic_cast<ImageLayer*>(i->layer)) {
                    ImVec2 crop(r->cropOffset.x, r->cropOffset.y);
                    ImVec2 pos = ImVec2(i->recursedOrigin.x, i->recursedOrigin.y) * _size;
                    ImVec2 size = ImVec2(i->recursedScale.x, i->recursedScale.y) * ImVec2(i->size.x, i->size.y) * _size;
                    ImVec2 cursorPos = ImVec2(pos.x,  -pos.y) - ImVec2((size.x * 0.5f) * i->xOffset, (size.y * 0.5f)  * i->yOffset);
                    
                    ImGui::SetCursorPos(cursorPos * _scale + ImVec2(m_stateManager.off_x, m_stateManager.off_y));
                    ImGui::Image((ImTextureID)i->layer->material.texture->_pTexture, size * _scale, ImVec2(0,0), ImVec2(1,1) );
                }
                else if (ParticleLayer* r = dynamic_cast<ParticleLayer*>(i->layer)) {
                    
                    r->update({i->recursedOrigin.x, i->recursedOrigin.y, 0});
                    
                    for (auto& inst : r->insts) {
                        ImVec2 pos = ImVec2(inst.pos.x, inst.pos.y) * _size;
                        ImVec2 size = ImVec2(i->recursedScale.x, i->recursedScale.y) * inst.size * _size;
                        ImVec2 cursorPos = ImVec2(pos.x,  -pos.y) - ImVec2((size.x * 0.5f) * i->xOffset, (size.y * 0.5f)  * i->yOffset);
                        
                        ImGui::SetCursorPos(cursorPos * _scale + ImVec2(m_stateManager.off_x, m_stateManager.off_y));
                        ImGui::Image((ImTextureID)i->layer->material.texture->_pTexture, size * _scale, ImVec2(0,0), ImVec2(1,1) );
                    }
                }
            }
            c++;
        }
    }
    
    ImGui::PopStyleVar();
    ImGui::End();
    
}
