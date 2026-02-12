#include "stdafx.hpp"
#include "R_Common.hpp"
#include "deps/SPIRV-Reflect/spirv_reflect.h"
#include "deps/glslang/glslang/Public/ShaderLang.h"
#include "deps/glslang/SPIRV/GlslangToSpv.h"
#include "deps/SPIRV-Cross/spirv_msl.hpp"
#include "deps/xxhash.h"

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

MTL::VertexFormat getMTLFormat(spirv_cross::SPIRType type) {
    switch (type.basetype) {
        case spirv_cross::SPIRType::Float:
            switch (type.vecsize) {
                case 1: return MTL::VertexFormatFloat;
                case 2: return MTL::VertexFormatFloat2;
                case 3: return MTL::VertexFormatFloat3;
                case 4: return MTL::VertexFormatFloat4;
            }
        case spirv_cross::SPIRType::UInt:
            switch (type.vecsize) {
                case 1: return MTL::VertexFormatUInt;
                case 2: return MTL::VertexFormatUInt2;
                case 3: return MTL::VertexFormatUInt3;
                case 4: return MTL::VertexFormatUInt4;
            }
        case spirv_cross::SPIRType::Int:
            switch (type.vecsize) {
                case 1: return MTL::VertexFormatInt;
                case 2: return MTL::VertexFormatInt2;
                case 3: return MTL::VertexFormatInt3;
                case 4: return MTL::VertexFormatInt4;
            }
        default: return MTL::VertexFormatInvalid;
    }
}


std::vector<uint64_t> _mtlVertexFormats;

std::string REND_StateManager::parseGLSLShaderDirective(const std::string &code)
{
    // the require directive
    return "";
}

std::string REND_StateManager::parseGLSLShader(const std::string &fileExt, const std::string &entryPoint, uint64_t shaderStage)
{
    _mtlVertexFormats = {};
    std::string rpath = GetBundleFilePath(fileExt);
    std::ifstream str(rpath.data());
    
    if (!str.is_open()) {
        assert( false );
        return "";
    }
    
    std::ostringstream buffer;
    buffer << str.rdbuf();
    
    std::string contents = buffer.str();
    str.close();
    
    auto * c_contents = contents.data();
    
    auto glslShaderStage = shaderStage == SHADER_STAGE_VERTEX ? EShLangVertex : EShLangFragment;
    
    
    glslang::TShader shader(glslShaderStage);
    shader.setStrings(&c_contents, 1);
    
    shader.setEnvInput(glslang::EShSourceGlsl, glslShaderStage, glslang::EShClientOpenGL, shader_version);
    shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_1);
    shader.setEntryPoint(entryPoint.data());
    shader.setAutoMapLocations(true);
    
    std::string preamble =
    "#extension GL_GOOGLE_include_directive : require\n"
    "#define mul(a, b) ((a) * (b))\n"
    "#define CAST2(x) vec2(x)\n"
    "#define CAST3(x) vec3(x)\n"
    "#define texSample2D(s, t) texture(s, t)\n"
    "#define saturate(x) clamp(x, 0.0, 1.0)\n"
    "#define frac(x) fract(x)\n"
    ;
    
    if (shaderStage == SHADER_STAGE_FRAGMENT) {
        preamble +=
        "out vec4 _FragColor; \n"
        "#define gl_FragColor _FragColor \n"
        ;
    }
    
    shader.setPreamble(preamble.data());
    
    TBuiltInResource resources;
    TIncluder includer;
    
    
    if (!shader.parse(&resources, shader_version, false, EShMsgDefault, includer)) {
        printf("GLSL parsing failed: %s\n", shader.getInfoLog());
        printf(" %s\n", shader.getInfoDebugLog());
        return "";
    }
    
    glslang::TProgram program;
    program.addShader(&shader);
    
    if (!program.link(EShMsgDefault)) {
        printf("%s GLSL linking failed\n", program.getInfoLog());
        printf("%s\n", program.getInfoDebugLog());
        return "";
    }
    
    std::vector<uint32_t> spirv;
    glslang::GlslangToSpv(*program.getIntermediate(glslShaderStage), spirv);
    
    spirv_cross::CompilerMSL mslCompiler(spirv);
    spirv_cross::CompilerMSL::Options mslOptions;
    
    
    mslOptions.set_msl_version(3, 0);
    mslCompiler.set_msl_options(mslOptions);
    
    spirv_cross::ShaderResources shaderResources = mslCompiler.get_shader_resources();
    
    
    if (shaderStage == SHADER_STAGE_VERTEX)
    {
        mslCompiler.rename_entry_point("main", "vsmain", spv::ExecutionModelVertex);

        int i = 0;
        for (auto &resource : shaderResources.stage_inputs) {
            uint32_t location = mslCompiler.get_decoration(resource.id, spv::DecorationLocation);
            mslCompiler.set_decoration(resource.id, spv::DecorationLocation, i++);
            
            const spirv_cross::SPIRType& type = mslCompiler.get_type(resource.type_id);
            _mtlVertexFormats.push_back(getMTLFormat(type));
        }
    }
    else
    {
        mslCompiler.rename_entry_point("main", "fsmain", spv::ExecutionModelFragment);
    }

    std::string mtlSource;
    try {
        mtlSource = mslCompiler.compile();
        printf("Generated METAL: \n%s\n", mtlSource.data());
    } catch (const spirv_cross::CompilerError& e) {
        glslang::FinalizeProcess();
    }
    
    return mtlSource;
}

void REND_StateManager::createShader(const std::string &file)
{
    std::vector<uint64_t> mtlVertexFormats;
    std::string pipelineCode = "";
    std::string vertCode = parseGLSLShader(file + ".vert", "main", SHADER_STAGE_VERTEX);
    mtlVertexFormats = _mtlVertexFormats;
    std::string fragCode = parseGLSLShader(file + ".frag", "main", SHADER_STAGE_FRAGMENT);
    
    if (vertCode.empty()  || fragCode.empty())
        return;
    
    pipelineCode = vertCode + "\n\n" + fragCode;
    
    MTLShader mtlShader = {};
    mtlShader.create(pipelineCode, "vsmain", "fsmain", {MTL::PixelFormatBGRA8Unorm}, mtlVertexFormats);
    
    shaders[file] = mtlShader;
}

void REND_StateManager::createShaders()
{
    glslang::InitializeProcess();
    
    createShader("assets/shaders/genericimage");
    
    glslang::FinalizeProcess();
}
