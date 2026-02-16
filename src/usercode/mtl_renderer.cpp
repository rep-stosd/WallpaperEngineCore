// mtl_renderer.cpp - metal presenter backend

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include "mtl_renderer.h"
#include "R_Common.hpp"
#include <fstream>
#include <sstream>

#define min(x, y) ( (x>y) ? y : x )
#define max(x, y) ( (x>y) ? x : y )

void* currentDrawable() ;
void* renderPassDescriptor() ;
void* metalLayer() ;
int metalLayerWidth();
int metalLayerHeight();

std::string GetBundleFilePath(const std::string& filename) ;

int calcVertexFormatStride(MTL::VertexFormat fmt)
{
    switch (fmt)
    {
        case MTL::VertexFormatUInt:
        case MTL::VertexFormatInt:
        case MTL::VertexFormatFloat:
            return sizeof(float) * 1;
        case MTL::VertexFormatUInt2:
        case MTL::VertexFormatInt2:
        case MTL::VertexFormatFloat2:
            return sizeof(float) * 2;
        case MTL::VertexFormatUInt3:
        case MTL::VertexFormatInt3:
        case MTL::VertexFormatFloat3:
            return sizeof(float) * 3;
        case MTL::VertexFormatUInt4:
        case MTL::VertexFormatInt4:
        case MTL::VertexFormatFloat4:
            return sizeof(float) * 4;
            
        default:
            break;
    }
    
    return 0;
}

void MTLShader::create(const std::string& code, const std::string& entryVS, const std::string& entryPS, const std::vector<uint64_t>& attachmentFmt, const std::vector<uint64_t>& vertexFmt, uint64_t depthFmt) {
    auto* pDevice = MTLRenderer::singleton()->_pDevice;
    
    auto NSCode = NS::String::string(code.data(), NS::UTF8StringEncoding);
    
    if (code.size() < 96) {
        std::string rpath = GetBundleFilePath(code);
        std::ifstream str(rpath.data());
        
        if (!str.is_open()) {
            assert( false );
        }
        
        std::ostringstream buffer;
        buffer << str.rdbuf();
        
        std::string contents = buffer.str(); // hold it in a variable
        NSCode = NS::String::string(contents.c_str(), NS::UTF8StringEncoding);
    }
    
    
    NS::Error* pError = nullptr;
    _pShaderLibrary = pDevice->newLibrary( NSCode, nullptr, &pError );
    
    if ( !_pShaderLibrary ) {
        __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
        assert( false );
    }
    

    MTL::Function* pVertexFn = _pShaderLibrary->newFunction( NS::String::string(entryVS.data(), NS::UTF8StringEncoding) );
    MTL::Function* pFragFn = _pShaderLibrary->newFunction( NS::String::string(entryPS.data(), NS::UTF8StringEncoding) );
    
    
    

    _pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    _pDesc->setVertexFunction( pVertexFn );
    _pDesc->setFragmentFunction( pFragFn );
    
    
    {
        _pDesc->colorAttachments()->object(0)->setBlendingEnabled(true);
        
        // RGB blending
        _pDesc->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
        _pDesc->colorAttachments()->object(0)->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
        _pDesc->colorAttachments()->object(0)->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
        
        // Alpha blending
        _pDesc->colorAttachments()->object(0)->setAlphaBlendOperation(MTL::BlendOperationAdd);
        _pDesc->colorAttachments()->object(0)->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
        _pDesc->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
    }
    
    
    if (attachmentFmt.size() > 0) {
        for (int i = 0; i < attachmentFmt.size(); i++)
            _pDesc->colorAttachments()->object(i)->setPixelFormat( (MTL::PixelFormat)attachmentFmt[i]);
    }
    else
        _pDesc->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatRGBA8Unorm );
    
    if (vertexFmt.size() > 0) {
        _pDesc->setVertexDescriptor(MTL::VertexDescriptor::vertexDescriptor());
        
        int offset = 0;
        for (int i = 0; i < vertexFmt.size(); i++)
        {
            _pDesc->vertexDescriptor()->attributes()->object(i)->setFormat((MTL::VertexFormat)vertexFmt[i]);
            _pDesc->vertexDescriptor()->attributes()->object(i)->setOffset(offset);
            _pDesc->vertexDescriptor()->attributes()->object(i)->setBufferIndex(16);
            
            offset += calcVertexFormatStride((MTL::VertexFormat)vertexFmt[i]);
        }
        
        _pDesc->vertexDescriptor()->layouts()->object(16)->setStride(offset);
        _pDesc->vertexDescriptor()->layouts()->object(16)->setStepFunction(MTL::VertexStepFunctionPerVertex);
    }
    
    if (depthFmt != UINT64_MAX) {
        _pDesc->setDepthAttachmentPixelFormat( (MTL::PixelFormat)depthFmt );
        if ( depthFmt == MTL::PixelFormatDepth24Unorm_Stencil8) {
#ifdef __arm64__
            _pDesc->setDepthAttachmentPixelFormat( MTL::PixelFormatDepth16Unorm );
            _pDesc->setStencilAttachmentPixelFormat(MTL::PixelFormatStencil8);
#else
            _pDesc->setStencilAttachmentPixelFormat(MTL::PixelFormatDepth24Unorm_Stencil8);
#endif
        }
    }

    _pPSO = pDevice->newRenderPipelineState( _pDesc, &pError );
    
    if (!_pPSO) {
        __builtin_printf( "%s\n", pError->localizedDescription()->utf8String() );
        assert( false );
    }
}

void MTLShader::destroy() {
    _pShaderLibrary->release();
    _pPSO->release();
    _pDesc->release();
}

void MTLTexture::create(uint64_t width, uint64_t height, uint64_t fmt, uint64_t type, uint64_t storageMode, uint64_t usage) {
    auto* pDevice = MTLRenderer::singleton()->_pDevice;
    
#ifdef __arm64__
    if (storageMode == MTL::StorageModeManaged)
        storageMode = MTL::StorageModeShared;
#endif
    
   _pDesc = MTL::TextureDescriptor::alloc()->init();
   _pDesc->setTextureType((MTL::TextureType)type);
   _pDesc->setStorageMode((MTL::StorageMode)storageMode );
   _pDesc->setPixelFormat((MTL::PixelFormat)fmt);
   _pDesc->setWidth(width);
   _pDesc->setHeight(height);
   _pDesc->setUsage(usage);
    if (type == MTL::TextureType2DArray)
        _pDesc->setArrayLength(4);

   _pTexture = pDevice->newTexture(_pDesc);
}

void MTLTexture::upload(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint64_t stride, uint8_t* textureData) {
    _pTexture->replaceRegion( MTL::Region( x, y, 0, w, h, 1 ), 0, textureData, w * stride );
}

void MTLTexture::upload(uint64_t stride, uint8_t* textureData) {
    _pTexture->replaceRegion( MTL::Region( 0, 0, 0, _pDesc->width(), _pDesc->height(), 1 ), 0, textureData, _pDesc->width() * stride );
}

void MTLTexture::uploadDirect(uint64_t stride, uint8_t* textureData) {
    _pTexture->replaceRegion( MTL::Region( 0, 0, 0, _pDesc->width(), _pDesc->height(), 1 ), 0, textureData, stride );
}

void MTLTexture::destroy() {
    _pTexture->release();
    _pDesc->release();
}

void MTLBuffer::create(uint64_t size, uint64_t storageMode) {
    auto* pDevice = MTLRenderer::singleton()->_pDevice;
    
#ifdef __arm64__
    if (storageMode == MTL::ResourceStorageModeManaged)
        storageMode = MTL::ResourceStorageModeShared;
#endif
    _pBuffer = pDevice->newBuffer( size, storageMode );
    this->size = size;
}

void MTLBuffer::upload(void* pData, uint64_t size, uint64_t offset) {
    memcpy( _pBuffer->contents(), (void*)((char*)pData + offset), size );
    
#ifndef __arm64__
    _pBuffer->didModifyRange( NS::Range::Make( 0, min(size, _pBuffer->length() ) ) );
#endif
    this->size = size;
}


void MTLBuffer::destroy() {
    _pBuffer->release();
}


void MTLDepthStencilState::create(uint64_t depthCmpFunc, bool depthEnable) {
    auto* pDevice = MTLRenderer::singleton()->_pDevice;
    
    _pDesc = MTL::DepthStencilDescriptor::alloc()->init();
    _pDesc->setDepthCompareFunction( (MTL::CompareFunction)depthCmpFunc );
    _pDesc->setDepthWriteEnabled( depthEnable );
    MTL::StencilDescriptor* xx = MTL::StencilDescriptor::alloc()->init();
    xx->setWriteMask(0xFF);
    xx->setReadMask(0xFF);
    xx->setStencilCompareFunction(MTL::CompareFunctionAlways);
    xx->setDepthStencilPassOperation(MTL::StencilOperationReplace);
    xx->setStencilFailureOperation(MTL::StencilOperationReplace);
    _pDesc->setFrontFaceStencil(xx);
    _pDesc->setBackFaceStencil(xx);

    _pDepthStencilState = pDevice->newDepthStencilState( _pDesc );

}

void MTLDepthStencilState::destroy() {
    _pDepthStencilState->release();
    _pDesc->release();
}



#include "deps/imgui/imgui_impl_metal.h"

MTLRenderer* MTLRenderer::_pSingleton = nullptr;

void MTLRenderer::init(MTL::Device *device) {
    _pDevice = device;
    _pCommandQueue = _pDevice->newCommandQueue();
    _semaphore = dispatch_semaphore_create( 5 );
    ImGui_ImplMetal_Init(_pDevice);
}

void MTLRenderer::renderTahoePipeline(MTL::CommandBuffer* pCmd, CA::MetalDrawable* pDrawable) {
    static bool _testInit = false;
    if (!_testInit) {
        m_stateManager.init();
        _testInit = true;
    }
    
    m_stateManager.frame();
    
    MTL::RenderPassDescriptor* pRpd = MTL::RenderPassDescriptor::alloc()->init();
    MTL::RenderPassColorAttachmentDescriptor* cd = pRpd->colorAttachments()->object(0);
    cd->setTexture(pDrawable->texture());
    cd->setLoadAction(MTL::LoadActionLoad);
    cd->setStoreAction(MTL::StoreActionStore);
    
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );
    
    
    
    
    ImGui_ImplMetal_NewFrame(pRpd);
    ImGui::NewFrame();
    
    
    void NetGameImguiWindow();
    NetGameImguiWindow();
    
    
    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), pCmd, pEnc);

    pEnc->endEncoding();
    pRpd->release();
}

NS::AutoreleasePool* MTLRenderer_pAutoreleasePool;
void MTLRenderer::beginFrame() {
    MTLRenderer_pAutoreleasePool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
    CA::MetalDrawable* pDrawable = (CA::MetalDrawable*)currentDrawable();
    
    state_pCmd = pCmd;
    state_pDrawable = pDrawable;
    
    dispatch_semaphore_wait( _semaphore, DISPATCH_TIME_FOREVER );
    MTLRenderer* pRenderer = this;
    pCmd->addCompletedHandler( ^void( MTL::CommandBuffer* pCmd ){
        dispatch_semaphore_signal( pRenderer->_semaphore );
    });
    
    renderTahoePipeline(pCmd, pDrawable);
    
    pCmd->presentDrawable( pDrawable );
    pCmd->commit();
    
    state_pCmd = nullptr;
    state_pDrawable = nullptr;
    
    MTLRenderer_pAutoreleasePool->release();
}

void MTLRenderer::endFrame() {
}

void MTLRenderer::destroy() {
    
}
