#pragma once

#include "stdafx.hpp"
#include "mtl_renderer.h"

#include <FreeImage.h>

// custom fif formats go here
#define FIF_MP4 FREE_IMAGE_FORMAT(99)

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
    
};

struct PAKModel {
    std::string name;
};

PAKImage PAKImage_Alloc(const std::string& imgPath);
void PAKImage_Free(PAKImage& img);
void PAKImage_GLUpload(PAKImage& bzImage, MTLTexture& texture);

// pakFile.cpp
int PAKFile_LoadAndDecompress(const char *filename);


inline std::string Wallpaper64GetStorageDir() {
    return std::string(getenv("HOME")) + "/.wallpaperio/";
}

