
#include "R_Common.hpp"
#include "deps/SPIRV-Reflect/spirv_reflect.h"
#include "deps/glslang/glslang/Public/ShaderLang.h"
#include "deps/glslang/SPIRV/GlslangToSpv.h"
#include "deps/SPIRV-Cross/spirv_msl.hpp"

const char *TEST_SHADER_CODE = R"(

cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
    matrix model;
    matrix view;
    matrix projection;
};


struct VS_INPUT
{
    float3 vPosition     : POSITION;
    float2 vUv            : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 vPosition      : SV_POSITION;
    float2 vUv             : UV;
    float3 vNormal        : NORMAL;
};

Texture2D tColormap : register( t0 );
Texture2D tDetail : register( t1 );

VS_OUTPUT mainVS( VS_INPUT input )
{
    VS_OUTPUT output;
    output.vPosition = float4(input.vPosition, 1.0f); //mul( worldToView, float4( input.vPosition, 0.0 ) );
    //output.vPosition = mul( viewToProjection, float4( output.vPosition.xyz, 1.0 ) );
    output.vPosition = mul(output.vPosition, model);
    output.vPosition = mul(output.vPosition, view);
    output.vPosition = mul(output.vPosition, projection);
//    output.vPosition.z = 0.0001;

    output.vUv = input.vUv;
    output.vNormal = normalize( input.vPosition.xyz );
    
    return output;
};

float4 mainPS( VS_OUTPUT input ) : SV_TARGET0
{
    return float4( 0.0, 1.0, 1.0, 1.0);
}
 
)";

void REND_StateManager::createShaders() {
    glslang::InitializeProcess();
    
    glslang::TShader shader(EShLangVertex);
    shader.setStrings(&TEST_SHADER_CODE, 1);
    
    shader.setEnvInput(glslang::EShSourceHlsl, EShLangVertex, glslang::EShClientVulkan, 100);
    shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_1);
    shader.setEntryPoint("mainVS");
    
    
    TBuiltInResource resources;
    
    if (!shader.parse(&resources, 100, false, EShMsgDefault)) {
        printf("HLSL parsing failed: %s\n", shader.getInfoLog());
        printf(" %s\n", shader.getInfoDebugLog());
        glslang::FinalizeProcess();
        return;
    }

    
    glslang::TProgram program;
    program.addShader(&shader);
    
    
    if (!program.link(EShMsgDefault)) {
        printf("%s HLSL linking failed\n", program.getInfoLog());
        printf("%s\n", program.getInfoDebugLog());
        glslang::FinalizeProcess();
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
    
    
    glslang::FinalizeProcess();
}
