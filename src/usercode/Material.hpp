#pragma once

#include "mtl_renderer.h"
#include "PAK.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

class MaterialComponent;

struct Material {
    MTLTexture* texture;
    int width, height;
    int texWidth, texHeight;
    std::vector<MaterialComponent*> components;
    
};

class MaterialComponent {
public:
    MaterialComponent(Material& mat) {
        _pMaterial = &mat;
        _pMaterial->components.push_back(this);
    }
    virtual void init() = 0;
    virtual void update() = 0;
    virtual void destroy() = 0;
protected:
    Material* _pMaterial;
};

class MatVideoComponent : public MaterialComponent {
public:
    MatVideoComponent(Material& mat) : MaterialComponent(mat) {}
    
    void init() override {};
    void init(const PAKImage& image);
    void init(const std::string& filename);
    void update() override;
    void destroy() override;
    
private:
    SwsContext* converter_ctx = nullptr;
    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVFrame* frame, *frame2;
    int video_stream_idx = -1;
    
    double frame_duration_sec = 0.0;
    double time_accumulator = 0.0;
    long long last_update_time_ms = 0;
};

