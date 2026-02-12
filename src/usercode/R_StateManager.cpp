#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "R_Common.hpp"
#include "CScene.hpp"
extern Scene scene;

int metalLayerWidth();
int metalLayerHeight();

REND_StateManager m_stateManager;

void REND_StateManager::init() {
    createShaders();
}


void REND_StateManager::calcRefDef()
{
    if (refdef.x != metalLayerWidth() || refdef.y != metalLayerWidth())
    {
        refdef.x = metalLayerWidth();
        refdef.y = metalLayerHeight();
        refdef.z = 1.f / refdef.x;
        refdef.w = 1.f / refdef.y;
    }
}


void REND_StateManager::frame() {
    calcRefDef();
    
    scene.update(); // todo: this shouldnt be here
    
    MTLRenderer * rend = MTLRenderer::singleton();
    MTL::CommandBuffer* pCmd = rend->state_pCmd;
    CA::MetalDrawable* pDrawable = rend->state_pDrawable;
    
    static MTL::SamplerState * _pSamplerState;
    static bool not_init = false;
    float number = 1.f;
    float mat4x4[] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,0.f,0.f,1.f};
    float vertices[] = {
        0.f, 0.f,  0.f, 0.f, 0.f,
        .25f, 0.f, 0.f, 1.f, 0.f,
        0.f, .25f, 0.f, 0.f, -1.f,
        0.f, .25f, 0.f, 0.f, -1.f,
        .25f, 0.f, 0.f, 1.f, 0.f,
        .25f, .25f, 0.f, 1.f, -1.f
    };
    
    
    if (!not_init)
    {
        MTL::SamplerDescriptor* pSamplerDesc = MTL::SamplerDescriptor::alloc()->init();
        pSamplerDesc->setMinFilter(MTL::SamplerMinMagFilterLinear);
        pSamplerDesc->setMagFilter(MTL::SamplerMinMagFilterLinear);
        pSamplerDesc->setSAddressMode(MTL::SamplerAddressModeRepeat);
        pSamplerDesc->setTAddressMode(MTL::SamplerAddressModeRepeat);

        _pSamplerState = rend->_pDevice->newSamplerState(pSamplerDesc);
        pSamplerDesc->release();
        
        not_init = true;
    }
    
    
    MTL::RenderPassDescriptor* pRpd = MTL::RenderPassDescriptor::alloc()->init();
    MTL::RenderPassColorAttachmentDescriptor* cd = pRpd->colorAttachments()->object(0);
    cd->setTexture(pDrawable->texture());
    cd->setLoadAction(MTL::LoadActionClear);
    cd->setStoreAction(MTL::StoreActionStore);
    cd->setClearColor(MTL::ClearColor(1.f, 1.f, 0.f, 1.f));
    
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );
    
    
    pEnc->setRenderPipelineState(shaders["assets/shaders/genericimage"]._pPSO);
    if (scene.textures.size() && scene.textures.begin()->second)
        pEnc->setFragmentTexture(scene.textures.begin()->second->_pTexture, 0);
    pEnc->setFragmentSamplerState(_pSamplerState, 0);
    
    glm::mat4 projection = glm::perspective(89.f, 1.6f, 0.1f, 2000.f);
    projection = glm::transpose(projection);
    
    pEnc->setFragmentBytes((void*)&number, 4, 0);
    pEnc->setFragmentBytes((void*)&number, 4, 1);
    pEnc->setFragmentBytes((void*)&number, 4, 2);
    
    pEnc->setVertexBytes((void*)&mat4x4, 4*4*4, 0);
    pEnc->setVertexBytes((void*)&number, 4, 1);
    pEnc->setVertexBytes((void*)&number, 4, 2);
    pEnc->setVertexBytes((void*)&number, 4, 3);
    
    pEnc->setCullMode(MTL::CullModeNone);
    
    int x = 0;
    for (auto& obj : scene.nodes) {
        
        if (cnt <= x)
            break;
        
        float _size = m_stateManager.sz_x;
        float _scale = m_stateManager.sz_y;
        
        auto pos = obj->recursedOrigin * _size;
        auto size = obj->recursedScale * glm::vec2(obj->size.x, obj->size.y) * _size;
        auto cursorPos = pos - glm::vec2((size.x * 0.5f) * obj->xOffset, (size.y * 0.5f)  * obj->yOffset) + glm::vec2(off_x, off_y);
        
        auto rtsize = glm::vec2(refdef.z, refdef.w);
        auto ori = cursorPos * rtsize;
        auto sz = ( cursorPos + size ) * rtsize;
        
        
        vertices[0] = ori.x; vertices[1] = ori.y; vertices[5] = sz.x; vertices[6] = ori.y;
        vertices[10] = ori.x; vertices[11] = sz.y; vertices[15] = ori.x; vertices[16] = sz.y;
        vertices[20] = sz.x; vertices[21] = ori.y; vertices[25] = sz.x; vertices[26] = sz.y;
        
        pEnc->setVertexBytes((void*)vertices, (4*3+4*2)*6, BUFFER_SLOT_VERTEX);
        
        if (obj->layer)
        {
            if (obj->layer->type != LAYER_TYPE_IMAGE)
                continue;
            else if (obj->layer->material.texture)
                pEnc->setFragmentTexture(obj->layer->material.texture->_pTexture, 0);
        }
        
        pEnc->drawPrimitives(MTL::PrimitiveTypeTriangle, 0, 6, 1, 0);
        
        x++;
    }
    
    pEnc->endEncoding();
    pRpd->release();
}
