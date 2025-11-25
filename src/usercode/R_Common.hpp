#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "mtl_renderer.h"

enum EBlendMode : uint32_t {
    kBlendModeDisabled,
    kBlendModeNormal = 0,
    kBlendModeAdditive,
    kBlendModeTranslucent
};

struct CMDPassData {
// depthstencil, rasterizer, etc
};

struct CMDPipelineData {
// vertex, index, uniform
    MTLBuffer vtxBuffer;
    MTLBuffer idxBuffer;
};

struct CMDInstanceData {
// instance
};

class REND_CommandList {
public:
    void setDepthEnabled(bool);
    void setBlendMode(EBlendMode);
    void setShader();
    void addQuad();
    void addModel();
private:
    
};

struct ShaderStateBuffer {
    struct ShaderStateInfo {
        uint64_t size;
        uint64_t offset;
    };
    std::vector<uint8_t> data;
    std::unordered_map<std::string, ShaderStateInfo> reflection;
    MTLBuffer gpuBuffer;
    uint64_t stateSize;
    
    void create(uint64_t size) {
        gpuBuffer.create(size, MTL::ResourceStorageModeManaged);
        stateSize = size;
    }
    
    void set(const std::string& name, const uint8_t *pdata) {
        auto &info = reflection[name];
        memcpy((void*)(data.data() + info.offset), (void*)pdata, info.size);
    }
    
    void commit() {
        gpuBuffer.upload((void*)data.data(), stateSize);
    }
    
    MTLBuffer& get() {
        return gpuBuffer;
    }
};


class REND_MetalPresenter {
public:
    void init(MTLRenderer& rend);
    void present();
    
private:
    MTLRenderer *_pRend;
};


class REND_StateManager {
public:
    /* R_ShaderParser.cpp */
    void createShaders();
    
    
    /* R_StateManager.cpp */
    void init();
    
private:
    std::unordered_map<uint64_t, ShaderStateBuffer> shaderUniformBuffers;
    std::unordered_map<uint64_t, MTLShader> shaders;
};

