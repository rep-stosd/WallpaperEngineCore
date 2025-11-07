#include "mtl_renderer.h"

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "gui/imgui_impl_glfw.h"

GLFWwindow* window = nullptr;

void apphide(GLFWwindow* wnd, void* metalDevice) ;

int main( int argc, char* argv[] )
{

    NS::AutoreleasePool* pAutoreleasePool = NS::AutoreleasePool::alloc()->init();
    
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    
    if (!glfwInit())
        return 0;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1920, 1080, "wallpaper64", NULL, NULL);
    
    auto* _pDevice = MTL::CreateSystemDefaultDevice();
    apphide(window, (void*)_pDevice);

    MTLRenderer::initSingleton();
    MTLRenderer::singleton()->init(_pDevice->retain());
    
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    //ImGui::GetIO().Fonts->AddFontFromFileTTF(GetBundleFilePath("cn.ttf").data(), 16.f, nullptr, ImGui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon());
    ImGui::GetIO().Fonts->AddFontFromFileTTF("/System/Library/Fonts/PingFang.ttc", 16.f);
   // ImGui::GetIO().Fonts->AddFontFromFileTTF(GetBundleFilePath("helvetica.ttc").data(), 16.f);

    
    while (!glfwWindowShouldClose(window)) {
        
        glfwPollEvents();
        ImGui_ImplGlfw_NewFrame();
        
        
        MTLRenderer::singleton()->beginFrame();
        MTLRenderer::singleton()->endFrame();
    }
    
    pAutoreleasePool->release();

    return 0;
}
