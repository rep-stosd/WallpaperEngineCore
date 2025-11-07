#pragma once

#include <cstdint>
#include <string>

class MTLTexture;
struct FIBITMAP;

class TextureLoader {
public:
    static void init();
    static MTLTexture createUncompressedTexture(const std::string& filePath, uint64_t fmt = 0);
    static void destroy();
};


