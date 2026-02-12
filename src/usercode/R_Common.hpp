#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "mtl_renderer.h"
#include "deps/glm/glm.hpp"
#include "deps/glm/gtc/matrix_transform.hpp"

#define SHADER_STAGE_VERTEX   0
#define SHADER_STAGE_FRAGMENT 1

#define BUFFER_SLOT_VERTEX 16
#define BUFFER_SLOT_INSTANCE 17

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
    
    void init();
private:
    
    
};

struct UniformBuffer {
    struct ShaderStateInfo {
        uint64_t size;
        uint64_t offset;
    };
    std::vector<uint8_t> data;
    std::unordered_map<std::string, ShaderStateInfo> reflection;
    MTLBuffer gpuBuffer;
    uint64_t stateSize;
    
    void create(uint64_t size) {
        data.resize(size);
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


class REND_StateManager {
public:
    /* R_ShaderParser.cpp */
    std::string parseGLSLShaderDirective(const std::string& code);
    std::string parseGLSLShader(const std::string& fileExt, const std::string& entryPoint, uint64_t shaderStage);
    void createShader(const std::string& file);
    void createShaders();
    
    
    /* R_StateManager.cpp */
    void init();
    void frame();
    
    void calcRefDef();
    void frameCamera();
    
    float off_x, off_y;
    float sz_x, sz_y;
    int cnt;
private:
    std::unordered_map<std::string, UniformBuffer> shaderUniformBuffers;
    std::unordered_map<std::string, MTLShader> shaders;
    
    glm::mat4 view;
    glm::mat4 projection;
    
    glm::mat4 mvp;
    
    glm::vec4 refdef; // xy: resolution, zw: pixel width
};


extern REND_StateManager m_stateManager;
extern REND_CommandList m_cmdList;
