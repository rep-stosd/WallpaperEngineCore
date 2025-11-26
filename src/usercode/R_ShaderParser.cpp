#include "stdafx.hpp"
#include "R_Common.hpp"
#include "deps/SPIRV-Reflect/spirv_reflect.h"
#include "deps/glslang/glslang/Public/ShaderLang.h"
#include "deps/glslang/SPIRV/GlslangToSpv.h"
#include "deps/SPIRV-Cross/spirv_msl.hpp"

const uint64_t shader_version = 330;

class TIncluder : public glslang::TShader::Includer {
public:
    IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) override {
        return readLocalPath(headerName, includerName, (int)inclusionDepth);
    }

    IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t inclusionDepth) override {
        return readLocalPath(headerName, includerName, (int)inclusionDepth);
    }

    void releaseInclude(IncludeResult* result) override {
        if (result) {
            delete[] result->headerData;
            delete result;
        }
    }

private:
    IncludeResult* readLocalPath(const char* headerName, const char* includerName, int depth) {
        // in the future, adjust for workshop shaders (not in base resource)
        std::string rpath = headerName;
        std::ifstream file("assets/shaders/" + rpath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            return nullptr;
        }

        size_t fileSize = (size_t)file.tellg();
        char* content = new char[fileSize + 1];
        file.seekg(0);
        file.read(content, fileSize);
        content[fileSize] = '\0';

        return new IncludeResult(headerName, content, fileSize, nullptr);
    }
};

void REND_StateManager::parseGLSLShader(const std::string &file, const std::string &entryPoint, uint64_t shaderStage){
    std::string rpath = GetBundleFilePath(file);
    std::ifstream str(rpath.data());
    
    if (!str.is_open()) {
        assert( false );
    }
    
    std::ostringstream buffer;
    buffer << str.rdbuf();
    
    std::string contents = buffer.str();
    str.close();
    
    auto * c_contents = contents.data();
    
    
    glslang::TShader shader(EShLangVertex);
    shader.setStrings(&c_contents, 1);
    
    shader.setEnvInput(glslang::EShSourceGlsl, EShLangVertex, glslang::EShClientVulkan, shader_version);
    shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_1);
    shader.setEntryPoint(entryPoint.data());
    
    shader.setPreamble("#extension GL_GOOGLE_include_directive : require\n");
    
    
    TBuiltInResource resources;
    TIncluder includer;
    
    
    if (!shader.parse(&resources, shader_version, false, EShMsgDefault, includer)) {
        printf("HLSL parsing failed: %s\n", shader.getInfoLog());
        printf(" %s\n", shader.getInfoDebugLog());
        return;
    }
    
    glslang::TProgram program;
    program.addShader(&shader);
    
    if (!program.link(EShMsgDefault)) {
        printf("%s HLSL linking failed\n", program.getInfoLog());
        printf("%s\n", program.getInfoDebugLog());
        return;
    }
    
    std::vector<uint32_t> spirv;
    glslang::GlslangToSpv(*program.getIntermediate(EShLangVertex), spirv);
    
    spirv_cross::CompilerMSL mslCompiler(spirv);
    spirv_cross::CompilerMSL::Options mslOptions;
    mslOptions.set_msl_version(2, 0);
    mslCompiler.set_msl_options(mslOptions);

    try {
        std::string mslSource = mslCompiler.compile();
        printf("Generated MSL: %s\n", mslSource.data());
    } catch (const spirv_cross::CompilerError& e) {
        glslang::FinalizeProcess();
        return;
    }
}

void REND_StateManager::createShaders() {
    glslang::InitializeProcess();
    
   
    parseGLSLShader("assets/shaders/genericimage.vert", "main", 114514);
    
    
    
    glslang::FinalizeProcess();
}
