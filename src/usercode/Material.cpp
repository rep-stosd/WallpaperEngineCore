#include "Material.hpp"

void MatVideoComponent::init(const PAKImage& img) {
    
    avformat_open_input(&format_ctx, "null", nullptr, nullptr);
    avformat_find_stream_info(format_ctx, nullptr);

    for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }

    AVCodecParameters* codecpar = format_ctx->streams[video_stream_idx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, codecpar);

    avcodec_open2(codec_ctx, codec, nullptr);

    _pMaterial->width = codec_ctx->width;
    _pMaterial->height = codec_ctx->height;
    
    _pMaterial->texture = new MTLTexture();
    _pMaterial->texture->create(_pMaterial->width, _pMaterial->height, MTL::PixelFormatBGRA8Unorm, MTL::TextureType2D, MTL::StorageModeManaged, MTL::TextureUsageShaderRead);
    
    frame2 = av_frame_alloc();
    av_image_alloc(frame2->data, frame2->linesize, _pMaterial->width, _pMaterial->height, AV_PIX_FMT_RGB32, 32);
                
    frame = av_frame_alloc();


    //codec_ctx->time_base = av_make_q(1, 60000);
    
    
    AVRational framerate = av_guess_frame_rate(format_ctx, format_ctx->streams[video_stream_idx], NULL);
    frame_duration_sec = av_q2d({framerate.den, framerate.num});

    last_update_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}


void MatVideoComponent::init(const std::string& videoPath) {
    if (!std::filesystem::exists(videoPath))
        return;

    avformat_open_input(&format_ctx, videoPath.data(), nullptr, nullptr);
    avformat_find_stream_info(format_ctx, nullptr);

    for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }

    AVCodecParameters* codecpar = format_ctx->streams[video_stream_idx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, codecpar);

    avcodec_open2(codec_ctx, codec, nullptr);

    _pMaterial->width = codec_ctx->width;
    _pMaterial->height = codec_ctx->height;
    
    _pMaterial->texture = new MTLTexture();
    _pMaterial->texture->create(_pMaterial->width, _pMaterial->height, MTL::PixelFormatBGRA8Unorm, MTL::TextureType2D, MTL::StorageModeManaged, MTL::TextureUsageShaderRead);
    
    frame2 = av_frame_alloc();
    av_image_alloc(frame2->data, frame2->linesize, _pMaterial->width, _pMaterial->height, AV_PIX_FMT_RGB32, 32);
                
    frame = av_frame_alloc();


    //codec_ctx->time_base = av_make_q(1, 60000);
    
    
    AVRational framerate = av_guess_frame_rate(format_ctx, format_ctx->streams[video_stream_idx], NULL);
    frame_duration_sec = av_q2d({framerate.den, framerate.num});

    last_update_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}



void MatVideoComponent::update() {
    if (!format_ctx || frame_duration_sec <= 0.0) return;

    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    auto delta_ms = now_ms - last_update_time_ms;
    last_update_time_ms = now_ms;

    time_accumulator += (double)delta_ms / 1000.0;

    if (time_accumulator >= frame_duration_sec)
    {
        time_accumulator -= frame_duration_sec;
        
        AVPacket packet;
        
        while (av_read_frame(format_ctx, &packet) >= 0) {
            if (packet.stream_index == video_stream_idx) {
                if (avcodec_send_packet(codec_ctx, &packet) < 0) {
                    av_packet_unref(&packet);
                    break;
                }
                
                int ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    av_packet_unref(&packet);
                    continue;
                } else if (ret < 0) {
                    av_packet_unref(&packet);
                    break;
                }
                
                // --- Frame successfully decoded ---
                if (!converter_ctx) {
                    converter_ctx = sws_getContext(
                                                   codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
                                                   codec_ctx->width, codec_ctx->height, AV_PIX_FMT_BGRA,
                                                   SWS_BILINEAR, nullptr, nullptr, nullptr
                                                   );
                }
                
                if (converter_ctx) {
                    sws_scale(converter_ctx, frame->data, frame->linesize, 0, codec_ctx->height, frame2->data, frame2->linesize);
                    
                    _pMaterial->texture->upload(4, frame2->data[0]);
                }
                
                av_packet_unref(&packet);
                return;
            }
            av_packet_unref(&packet);
        }
        
        av_seek_frame(format_ctx, video_stream_idx, 0, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(codec_ctx);
        update();
    }
}

void MatVideoComponent::destroy() {
    av_freep(frame2->data);
    av_frame_free(&frame);
    av_frame_free(&frame2);


    sws_freeContext(converter_ctx);
    converter_ctx = nullptr;
    
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);

}
