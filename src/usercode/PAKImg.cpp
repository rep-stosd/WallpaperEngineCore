
#include "PAK.h"
#include "TextureLoader.hpp"

#include "deps/lz4.h"

uint32_t nextPowerOfTwo(uint32_t v) {
 //   if (v == 0) return 1;
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
}

PAKImage PAKImage_Alloc(const std::string& imgPath) {
    std::ifstream imgStream(imgPath.data());

    if (!imgStream.is_open()) {
        return {};
    }
    
    PAKImage img = {};
    PAKImageHeader imgHeader = {};

    imgStream.read(imgHeader.magic, 8);
    imgStream.ignore(1);
    imgStream.read(imgHeader.imageMagic, 8);
    imgStream.ignore(1);
    imgStream.read((char*)&imgHeader.textureFormat, sizeof(int));
    imgStream.read((char*)&imgHeader.textureFlags, sizeof(int));
    imgStream.read((char*)&imgHeader.textureWidth, sizeof(int));
    imgStream.read((char*)&imgHeader.textureHeight, sizeof(int));
    imgStream.read((char*)&imgHeader.width, sizeof(int));
    imgStream.read((char*)&imgHeader.height, sizeof(int));
    imgStream.read((char*)&imgHeader.__unk1, sizeof(int));
    imgStream.read(imgHeader.containerMagic, 8);
    imgStream.ignore(1);

    PAKImageContainerHeader003 containerHeader = {};

    int containerVersion = 3;
    imgStream.read((char*)&containerHeader.imageCount, sizeof(int));
  //  //printf("%s res %d %d %s \n", imgHeader.magic,  imgHeader.width, imgHeader.height, imgHeader.containerMagic);
    switch (imgHeader.containerMagic[7]) {
        case '1':
        case '2':
            containerHeader.fiFormat = FIF_UNKNOWN;
            break;
        case '4':
            imgStream.read((char*)&containerHeader.fiFormat, sizeof(FREE_IMAGE_FORMAT));
            imgStream.read((char*)&containerHeader.mp4, sizeof(uint32_t));
           // containerHeader.mp4 = true;
            if (containerHeader.mp4 || (imgHeader.textureFlags & PAKIMG_FLAG_MP4) )
                containerHeader.fiFormat = FIF_MP4;
            containerVersion = 4;
            break;
        case '3':
            imgStream.read((char*)&containerHeader.fiFormat, sizeof(FREE_IMAGE_FORMAT));

          //  imgStream.read((char*)&containerHeader.__unk1, sizeof(int));

           
            break;
    }
    
    for (int i = 0 ; i < containerHeader.imageCount ; i++) {

        imgStream.read((char*)&img.mipmapCount, sizeof(uint32_t));
        img.mips = new PAKImageMipmapEntry[img.mipmapCount];
        img.texWidth = imgHeader.textureWidth;
        img.texHeight = imgHeader.textureHeight;
        img.width = imgHeader.width;
        img.height = imgHeader.height;
        img.fiFormat = containerHeader.fiFormat;
        img.internalFormat = imgHeader.textureFormat;

        for (int j = 0; j < img.mipmapCount ; j++ ) {
            PAKImageMipmapEntry mipEntry = {};
            if (containerHeader.mp4 || (containerVersion == 4 && containerHeader.fiFormat == FIF_UNKNOWN)) {
         //      imgStream.ignore(sizeof(uint32_t));
          //     imgStream.ignore(sizeof(uint32_t));
               //imgStream.ignore(sizeof(uint32_t));
            }
            imgStream.read((char*)&mipEntry.width, sizeof(uint32_t));
            imgStream.read((char*)&mipEntry.height, sizeof(uint32_t));
            
            if (containerVersion >= 2) {
                imgStream.read((char*)&mipEntry.compressed, sizeof(uint32_t));
                imgStream.read((char*)&mipEntry.realSize, sizeof(uint32_t));
            }
            
            imgStream.read((char*)&mipEntry.compressedSize, sizeof(uint32_t));

            if (mipEntry.compressed) {

              uint8_t* compressedData = new uint8_t[mipEntry.compressedSize];
              imgStream.read((char*)compressedData, mipEntry.compressedSize);

              mipEntry.data = new uint8_t[mipEntry.realSize];

              LZ4_decompress_safe((char*)compressedData, (char*)mipEntry.data, mipEntry.compressedSize,
                mipEntry.realSize);
              
              delete[] compressedData;

            }
            else {
              mipEntry.realSize = mipEntry.compressedSize;

              mipEntry.data = new uint8_t[mipEntry.compressedSize];
              imgStream.read((char*)mipEntry.data, mipEntry.compressedSize);
            }

            //printf("%d imgres %d %d sz %u %u \n", containerHeader.imageCount, mipEntry.width, mipEntry.height, mipEntry.realSize, mipEntry.compressedSize );

            img.mips[j] = mipEntry;

        }
    }

    img.name = imgPath;
    
    imgStream.close();
    return img;
} 


void PAKImage_Free(PAKImage& img) {
  if (img.mips) {
    for (int i = 0; i < img.mipmapCount; i++) {
      if (img.mips[i].data) {
        delete[] img.mips[i].data;
      }
    }
      delete[] img.mips;
  }
}


void PAKImage_GLUpload_PNG(PAKImage& bzImage, MTLTexture& texture) {
    FIMEMORY* data = FreeImage_OpenMemory(bzImage.mips[0].data, bzImage.mips[0].realSize);
    FIBITMAP *fiRawData = FreeImage_LoadFromMemory(bzImage.fiFormat, data);
    FIBITMAP *fiBitmap = FreeImage_ConvertTo32Bits(fiRawData);
    
    if (fiBitmap) {
	    FreeImage_FlipVertical(fiBitmap);
  //      bzImage.width = nextPowerOfTwo(bzImage.width);
  //      bzImage.height = nextPowerOfTwo(bzImage.height);
  //      auto _fiBitmap = FreeImage_Rescale(fiBitmap, bzImage.width, bzImage.height, FILTER_BILINEAR);
  //      FreeImage_Unload(fiBitmap);
  //      fiBitmap = _fiBitmap;
    }

    texture.create(bzImage.width, bzImage.height, MTL::PixelFormatBGRA8Unorm, MTL::TextureType2D, MTL::StorageModeManaged, MTL::TextureUsageShaderRead);
    texture.upload(4, (uint8_t*)FreeImage_GetBits(fiBitmap));
    
    FreeImage_Unload(fiBitmap);
    FreeImage_Unload(fiRawData);
    FreeImage_CloseMemory(data);
}

// you MUST be very careful with raw uploads, any non-freeimage format are considered raw and that includes video/sound files
//   which are not yet parsed
void PAKImage_GLUpload_RAW(PAKImage& bzImage, MTLTexture& texture) {
    texture.create(bzImage.texWidth, bzImage.texHeight, MTL::PixelFormatRGBA8Unorm, MTL::TextureType2D, MTL::StorageModeManaged, MTL::TextureUsageShaderRead);
    texture.upload(4, (uint8_t*)bzImage.mips[0].data);
}

void PAKImage_GLUpload_RAW_R8(PAKImage& bzImage, MTLTexture& texture) {
    texture.create(bzImage.texWidth, bzImage.texHeight, MTL::PixelFormatR8Unorm, MTL::TextureType2D, MTL::StorageModeManaged, MTL::TextureUsageShaderRead);
    texture.upload(1, (uint8_t*)bzImage.mips[0].data);
}


void PAKImage_GLUpload_RAW_RG88(PAKImage& bzImage, MTLTexture& texture) {
    texture.create(bzImage.texWidth, bzImage.texHeight, MTL::PixelFormatRG8Unorm, MTL::TextureType2D, MTL::StorageModeManaged, MTL::TextureUsageShaderRead);
    texture.upload(2, (uint8_t*)bzImage.mips[0].data);
}

// 0 => ARGB8888, 4 => DXT5, 6 => DXT3, 7 => DXT1, 8 => RG88, 9 => R8
void PAKImage_GLUpload_DDS(PAKImage& bzImage, MTLTexture& texture, int type) {
    uint64_t format = MTL::PixelFormatBC3_RGBA;
    int blockSize = 16;
    switch(type) {
    case 0:
      format = MTL::PixelFormatBC3_RGBA;
      break;
    case 1:
      format = MTL::PixelFormatBC3_RGBA;
      break;
    case 2:
      blockSize = 8;
      format = MTL::PixelFormatBC1_RGBA;
      break;
    };

    auto size = ((bzImage.mips[0].width+3)/4)  * blockSize;
    
    texture.create(bzImage.width, bzImage.height, format, MTL::TextureType2D, MTL::StorageModeManaged, MTL::TextureUsageShaderRead);
    texture.uploadDirect(size, (uint8_t*)bzImage.mips[0].data);
}

void PAKImage_GLUpload_MP4(PAKImage& bzImage)
{
    
}


void PAKImage_GLUpload(PAKImage& bzImage, MTLTexture& texture) {
   // printf("%.2f %.2f\n", (float)bzImage.width/bzImage.texWidth, (float)bzImage.height/bzImage.texHeight);
    texture = {};
    if (bzImage.fiFormat == FIF_UNKNOWN) {

      //printf("WARNING: internalFormat %d \n", bzImage.internalFormat);
      // 0 => ARGB8888, 4 => DXT5, 6 => DXT3, 7 => DXT1, 8 => RG88, 9 => R8
      switch (bzImage.internalFormat)
      {
      case 0:
        
        PAKImage_GLUpload_RAW(bzImage, texture);
        return;

      case 4:
        PAKImage_GLUpload_DDS(bzImage, texture, 0);
        return;

      case 6:
        PAKImage_GLUpload_DDS(bzImage, texture, 1);
        return;
      
      case 7:
        PAKImage_GLUpload_DDS(bzImage, texture, 2);
        return;
      case 8:
        PAKImage_GLUpload_RAW_RG88(bzImage, texture);
        return;
      case 9:
        PAKImage_GLUpload_RAW_R8(bzImage, texture);
        return;
      
      default:
        return;
      }
    }
    else if (bzImage.fiFormat == FIF_MP4)
        PAKImage_GLUpload_MP4(bzImage);
    else 
        PAKImage_GLUpload_PNG(bzImage, texture);
}
