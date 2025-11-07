#include "mtl_renderer.h"
#include "TextureLoader.hpp"

#include <unordered_map>
#include <FreeImage.h>


std::string GetBundleFilePath(const std::string& filename) ;

std::unordered_map<std::string, FIBITMAP*> fiTextureMap;
std::unordered_map<std::string, MTLTexture> mtlTextureMap;

void TextureLoader::init() {
    FreeImage_Initialise();
}

MTLTexture TextureLoader::createUncompressedTexture(const std::string &filePath, uint64_t fmt) {
    
    if(mtlTextureMap.find(filePath) != mtlTextureMap.end()) {
        return mtlTextureMap[filePath];
    }
    
    MTLTexture texture = {};
    
    FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
    FIBITMAP* image = 0;

    unsigned int width(0), height(0), bpp(0);

    auto rpath = GetBundleFilePath(filePath);

        fif = FreeImage_GetFileType(filePath.c_str(), 0);
        if (fif == FIF_UNKNOWN)
            fif = FreeImage_GetFIFFromFilename(filePath.c_str());
    
        if (fif == FIF_UNKNOWN) {
            fif = FIF_TARGA;
            return {};
        }

        if (FreeImage_FIFSupportsReading(fif))
            image = FreeImage_Load(fif, rpath.c_str());
    
        if (!image) {
            return {};
        }
    
            FreeImage_FlipVertical(image);


        bpp = FreeImage_GetBPP(image);
        width = FreeImage_GetWidth(image);
        height = FreeImage_GetHeight(image);
    
        if ((bpp == 0) || (width == 0) || (height == 0)) {
            FreeImage_Unload(image);
            return {};
        }

    
    uint64_t format = 0;


        switch (bpp)
        {
        case 4:
        case 8:
        case 16:
        case 24:
        {
            bpp = 32;
            FIBITMAP* newImage = FreeImage_ConvertTo32Bits(image);
            FreeImage_Unload(image);
            image = newImage;
        }
        case 32:
            format = MTL::PixelFormatBGRA8Unorm;
            if (fif == FIF_DDS)
                format = MTL::PixelFormatBGRA8Unorm;
            break;
        case 128:
            {
                format = MTL::PixelFormatRGBA32Float;
            }
            break;
        default:
        {
            format = MTL::PixelFormatRGBA8Unorm;
        }
        break;
        }

    fiTextureMap[filePath] = image;
    
    
    texture.create(width, height, format, MTL::TextureType2D, MTL::StorageModeManaged, MTL::TextureUsageShaderRead);
    texture.upload(bpp/8, FreeImage_GetBits(image));
    
    mtlTextureMap[filePath] = texture;

    return texture;
}

void TextureLoader::destroy() {
    FreeImage_DeInitialise();
}
