#pragma once

#include "stdafx.hpp"
#include "mtl_renderer.h"

#include <FreeImage.h>

// custom fif formats go here
#define FIF_MP4 FREE_IMAGE_FORMAT(99)

//#define PAKIMG_FLAG_
#define PAKIMG_FLAG_MP4 (1<<5)

// pakImage.cpp
struct PAKImageHeader {
    char magic[8];
    uint8_t __padd1;
    char imageMagic[8];
    uint8_t __padd2;
    int textureFormat;
    int textureFlags;
    int textureWidth;
    int textureHeight;
    int width;
    int height;
    int __unk1;
    char containerMagic[8];
    uint8_t __padd3;
};

struct PAKImageContainerHeader001 {

};

struct PAKImageContainerHeader003 {
    int imageCount;
    FREE_IMAGE_FORMAT fiFormat;
    int mp4;
};

struct PAKImageContainerHeader004 {
    int mp4;
};

struct PAKImageMipmapEntry {
    uint32_t width;
    uint32_t height;
    uint32_t compressed;
    uint32_t realSize;
    uint32_t compressedSize;
    uint8_t* data;
};

struct PAKImage {
    std::string name;
    FREE_IMAGE_FORMAT fiFormat;
    uint32_t internalFormat;
    
    uint32_t width, height;
    uint32_t texWidth, texHeight;

    int mipmapCount;
    PAKImageMipmapEntry* mips;
};

// pakModel.cpp

struct PAKModelHeader {
    char magic[8];
    int flags;
    std::string json;
    
    char skinmagic[8];
};

struct Vertex {
    glm::vec3 position;
    glm::uvec4 blend_indices;
    glm::vec4 weight;
    glm::vec2 texcoord;
    
};


struct PAKModel {
    struct Bone
    {
        uint32_t parent = 0xFFFFFFFFu;
        glm::mat3x2 transform;
    };
    
    
    struct BoneFrame {
        glm::vec3 position;
        glm::vec3 angle;
        glm::vec3 scale;

        // prepared
        glm::vec4 quaternion;
    };
    
    struct Animation
    {
        std::string name;
        uint32_t id;
        float fps;
        int length;
        int mode;
        
        std::vector<std::vector<BoneFrame>> bframes_array;
        
        // prepared
        double max_time;
        double frame_time;
        struct InterpolationInfo {
            int    frame_a;
            int    frame_b;
            float t;
        };
        InterpolationInfo getInterpolationInfo(float* cur_time) const;
    };
    
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    std::vector<Bone> bones;
    std::vector<Animation> animations;
    
    int animVer;
    
};

PAKImage PAKImage_Alloc(const std::string& imgPath);
void PAKImage_Free(PAKImage& img);
void PAKImage_GLUpload(PAKImage& bzImage, MTLTexture& texture);

// pakFile.cpp
int PAKFile_LoadAndDecompress(const char *filename);

// pakMdl.cpp
PAKModel PAKModel_Alloc(const std::string& path);


inline std::string Wallpaper64GetStorageDir() {
    return std::string(getenv("HOME")) + "/.wallpaperio/";
}

