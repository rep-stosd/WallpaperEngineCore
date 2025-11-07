#pragma once

#include <vector>

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

class MTLRenderer {
public:
    void init(MTL::Device* device);
    
    void beginFrame();
    void renderCompositionPipeline(MTL::CommandBuffer* pCmd);
    void renderTahoePipeline(MTL::CommandBuffer* pCmd, CA::MetalDrawable* drawable);
    void endFrame();
    
    void destroy();
    
    static inline MTLRenderer* singleton() {
        return _pSingleton;
    }
    
    static inline void initSingleton() {
        if (!_pSingleton)
            _pSingleton = new MTLRenderer();
    }
    
    static MTLRenderer* _pSingleton;
    
    MTL::Device* _pDevice;
    MTL::CommandQueue* _pCommandQueue;
    dispatch_semaphore_t _semaphore;
};

class MTLShader {
public:
 //   void create(const std::string& pathVS, const std::string& pathPS, const std::string& entryVS, const std::string& entryPS);
    void create(const std::string& code, const std::string& entryVS, const std::string& entryPS, const std::vector<uint64_t>& attachmentFmt = {}, uint64_t depthFmt = UINT64_MAX);
    void destroy();
    MTL::RenderPipelineState* _pPSO;
    MTL::RenderPipelineDescriptor* _pDesc;
    MTL::Library* _pShaderLibrary;
};

class MTLTexture {
public:
    void create(uint64_t width, uint64_t height, uint64_t fmt, uint64_t type, uint64_t storageMode, uint64_t usage);
    void upload(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint64_t stride, uint8_t* textureData);
    void upload(uint64_t stride, uint8_t* textureData);
    void uploadDirect(uint64_t stride, uint8_t* textureData);
    void destroy();
    MTL::Texture* _pTexture;
    MTL::TextureDescriptor* _pDesc;
};

class MTLBuffer {
public:
    void create(uint64_t size, uint64_t storageMode);
    void upload(void* pData, uint64_t size, uint64_t offset = 0);
    void destroy();
    MTL::Buffer* _pBuffer;
    uint64_t size;
};

class MTLDepthStencilState {
public:
    void create(uint64_t depthCmpFunc, bool depthEnable);
    void destroy();
    MTL::DepthStencilDescriptor* _pDesc;
    MTL::DepthStencilState* _pDepthStencilState;
};
