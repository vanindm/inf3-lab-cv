#pragma once

#include <memory>

#include "Colorspaces.hpp"
#include "Histogram.hpp"
#include "Score.hpp"
#include <PATypes/HashMap.h>
#include <PATypes/PairTuple.h>
#include <PATypes/Sequence.h>

#include <GL/glew.h>
#include <optional>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libswscale/swscale.h>
}

#include "AVHelper.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "contrib/stb_image.h"

#define INBUF_SIZE 4096

namespace CCTV {
struct Dot {
    size_t x, y;
};

class IFrame {
  public:
    virtual std::shared_ptr<IRGBColor> GetPoint(const Dot &at) const = 0;
    virtual ~IFrame() {};
};

class IIFrame {
  public:
    virtual std::shared_ptr<PATypes::Sequence<IFrame>>
    GetSubFrame(Dot upperLeftCorner, Dot bottomRightCorner) const;
    virtual ~IIFrame() = 0;
};

class IGLTexture {
  public:
    virtual GLuint GetTexture() const = 0;
    ~IGLTexture() {};
};

class Frame : public IFrame {
    unsigned char *data;
    int width, height, channels;
    class GLTexture : public IGLTexture {
        GLuint texture;

      public:
        GLTexture(const Frame &frame) {
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame.width, frame.height, 0,
                         GL_RGB, GL_UNSIGNED_BYTE, frame.data);
        }
        virtual ~GLTexture() { glDeleteTextures(1, &texture); }
        virtual GLuint GetTexture() const { return texture; }
    };
    class FrameHistogram : IHistogram<IRGBColor, int> {
        PATypes::HashMap<IRGBColor &, int> storage;

      public:
        FrameHistogram(const Frame &frame) {
            for (size_t i = 0; i < frame.width; ++i) {
                for (size_t j = 0; j < frame.height; ++j) {
                    try {
                        storage.Add(*frame.GetPoint({i, j}),
                                    (storage.Get(*frame.GetPoint({i, j}))));
                    } catch (std::out_of_range &) {
                        storage.Add(*frame.GetPoint({i, j}), 1);
                    }
                }
            }
        }
        virtual int GetFrequency(IRGBColor &color) const {
            try {
                return storage.Get(color);
            } catch (std::out_of_range &) {
                return 0;
            }
        }
    };

  public:
    Frame() : width(0), height(0), channels(0) {}
    Frame(const Frame &frame)
        : width(frame.width), height(frame.height), channels(frame.channels) {
        this->data = (unsigned char *)malloc(frame.channels * frame.width *
                                             frame.height);
        for (int i = 0; i < frame.channels * frame.width * frame.height; ++i) {
            this->data[i] = frame.data[i];
        }
    }
    Frame(int width, int height, int channels, const unsigned char *data)
        : width(width), height(height), channels(channels) {
        this->data = (unsigned char *)malloc(channels * width * height);
        for (int i = 0; i < width * height * channels; ++i) {
            this->data[i] = data[i];
        }
    }
    Frame(Frame &&frame)
        : width(frame.width), height(frame.height), channels(frame.channels) {
        data = frame.data;
        frame.data = nullptr;
    }
    std::shared_ptr<IGLTexture> GetTexture() const {
        return std::make_shared<GLTexture>(*this);
    }
    virtual ~Frame() {
        if (data)
            stbi_image_free(data);
    }
    static std::shared_ptr<Frame> FromFile(const std::string &filename) {
        std::shared_ptr<Frame> newFrame = std::make_shared<Frame>(Frame());
        newFrame->data = stbi_load(filename.c_str(), &newFrame->width,
                                   &newFrame->height, &newFrame->channels, 3);
        newFrame->channels = 3;
        if (!newFrame->data) {
            throw std::runtime_error(stbi_failure_reason());
        }
        return newFrame;
    }
    const unsigned char *GetData() const { return data; }
    std::shared_ptr<IRGBColor> GetPoint(const Dot &at) const {
        std::shared_ptr<RGBColor> res =
            std::make_shared<RGBColor>(data[(at.x + at.y * width) * channels]);
        return res;
    }
    Frame delta(const Frame &b) const {
        Frame newFrame(*this);
        if (width != b.width || height != b.height || channels != b.channels)
            throw std::logic_error("кадры несовместимы для операции delta");
        for (int i = 0; i < width * height * channels; ++i) {
            newFrame.data[i] = std::abs((int)data[i] - b.data[i]);
        }
        return newFrame;
    }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    Frame XOR(const Frame &b) const {
        Frame newFrame(*this);
        if (width != b.width || height != b.height || channels != b.channels)
            throw std::logic_error("кадры несовместимы для операции XOR");
        for (int i = 0; i < width * height * channels; ++i) {
            newFrame.data[i] = data[i] ^ b.data[i];
        }
        return newFrame;
    }
    Frame AND(const Frame &b) const {
        Frame newFrame(*this);
        if (width != b.width || height != b.height || channels != b.channels)
            throw std::logic_error("кадры несовместимы для операции XOR");
        for (int i = 0; i < width * height * channels; ++i) {
            newFrame.data[i] = (data[i] & b.data[i]);
        }
        return newFrame;
    }
    Frame Map(IRGBColor &(*f)(const IRGBColor &a)) const {
        Frame newFrame(*this);
        for (int i = 0; i < width * height; i += channels) {
            IRGBColor &res = f(RGBColor(newFrame.data + i));
            newFrame.data[i] = res.GetR();
            newFrame.data[i + 1] = res.GetG();
            newFrame.data[i + 2] = res.GetB();
        }
        return newFrame;
    }
    template <class T>
    T Reduce(T (*f)(const T &, const IRGBColor &), IRGBColor &init) const {
        T result = f(T(0), init);
        for (int i = 0; i < width * height; i += channels) {
            result = f(result, RGBColor(data + i));
        }
        return result;
    }
    double norm() const {
        double res = 0;
        for (int i = 0; i < width * height * channels; ++i) {
            res += data[i];
        }
        res /= (width * height);
        return res;
    }
    Frame &operator=(const Frame &other) {
        if (this == &other)
            return *this;
        if (data && width && height)
            free(data);
        this->data = (unsigned char *)malloc(other.channels * other.width *
                                             other.height);
        for (int i = 0; i < other.channels * other.width * other.height; ++i) {
            this->data[i] = other.data[i];
        }
        this->width = other.width;
        this->height = other.height;
        this->channels = other.channels;
        return *this;
    }
};

class FrameSequence : public PATypes::MutableListSequence<Frame>,
                      public IScoreable {
    int windowLength;
    double GetDeltaScore() {
        int lastIndex = getLength() - 1;
        if (lastIndex < 0 || windowLength < 2) {
            return 0;
        }
        Frame result = get(lastIndex);
        for (int i = 1; i < windowLength; ++i) {
            result = result.AND(get(lastIndex - i));
        }
        return get(lastIndex).delta(result).norm();
    }
    double GetDeltaScore2(int r) {
        if (r < 0 || windowLength < 2) {
            return 0;
        }
        double result = 0;
        Frame lastFrame = get(r);
        for (int i = 1; i < windowLength; ++i) {
            result += lastFrame.delta(get(r - i)).norm();
            lastFrame = get(r - i);
        }
        return result / (double)windowLength;
    }
    PATypes::HashMap<int, double> cache;

  public:
    FrameSequence() : PATypes::MutableListSequence<Frame>(), cache() {}
    FrameSequence(Frame *items, int count, int windowLength)
        : PATypes::MutableListSequence<Frame>(items, count),
          windowLength(windowLength), cache() {}
    FrameSequence(PATypes::Sequence<Frame> &sequence, int windowLength)
        : PATypes::MutableListSequence<Frame>(sequence),
          windowLength(windowLength), cache() {}
    FrameSequence(int windowLength)
        : PATypes::MutableListSequence<Frame>(), windowLength(windowLength),
          cache() {}
    FrameSequence(Frame item, int windowLength)
        : PATypes::MutableListSequence<Frame>(), windowLength(windowLength),
          cache() {}
    static FrameSequence LoadFromVideo(const std::string &filename,
                                       int windowSize) {
        FrameSequence result(windowSize);

        const AVCodec *codec = NULL;
        AVFormatContext *fmt_ctx = NULL;
        AVCodecContext *dec_ctx = NULL;
        AVInputFormat *fmt = NULL;
        AVPacket *pkt = av_packet_alloc();
        AVFrame *frame = av_frame_alloc();
        int streamIndex;

        if (avformat_open_input(&fmt_ctx, filename.c_str(), NULL, NULL) < 0) {
            throw std::logic_error("Ошибка при загрузке видео");
        }

        if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
            throw std::logic_error("Ошибка при загрузке видео");
        }

        if (open_codec_context(filename, &streamIndex, &dec_ctx, fmt_ctx,
                               AVMEDIA_TYPE_VIDEO) < 0) {
            throw std::logic_error("Ошибка при загрузке видео");
        }
        AVStream *stream = fmt_ctx->streams[streamIndex];

        int width = dec_ctx->width;
        int height = dec_ctx->height;
        AVPixelFormat pix_fmt = dec_ctx->pix_fmt;
        int dst_linesize[4];
        AVFrame *frameRGB;
        uint8_t *buffer;

        unsigned char *data;

        // if (av_image_alloc(&data, dst_linesize, width, height, pix_fmt, 1) !=
        //     0) {
        //     std::logic_error("Ошибка при загрузке видео");
        // }

        int ret = 0;

        int numBytes = av_image_get_buffer_size(
            AV_PIX_FMT_RGB24, dec_ctx->width, dec_ctx->height, 1);

        struct SwsContext *sws_ctx = sws_getContext(
            dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt, dec_ctx->width,
            dec_ctx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

        while (av_read_frame(fmt_ctx, pkt) >= 0) {
            if (pkt->stream_index == streamIndex) {
                ret = avcodec_send_packet(dec_ctx, pkt);
                if (ret < 0) {
                    fprintf(stderr,
                            "Error submitting a packet for decoding (%s)\n",
                            av_err2str(ret));
                    return ret;
                }

                while (ret >= 0) {
                    ret = avcodec_receive_frame(dec_ctx, frame);
                    if (ret < 0) {
                        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                            ret = 0;
                            break;
                        }

                        fprintf(stderr, "Error during decoding (%s)\n",
                                av_err2str(ret));
                        return ret;
                    }

                    frameRGB = av_frame_alloc();
                    buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

                    av_image_fill_arrays(frameRGB->data, frameRGB->linesize,
                                         buffer, AV_PIX_FMT_RGB24,
                                         dec_ctx->width, dec_ctx->height, 1);

                    sws_scale(sws_ctx, (uint8_t const *const *)frame->data,
                              frame->linesize, 0, dec_ctx->height, frameRGB->data,
                              frameRGB->linesize);

                    result.append(Frame(dec_ctx->width, dec_ctx->height,
                                        3,
                                        (unsigned char *)frameRGB->data[0]));

                    av_frame_unref(frame);
                    av_frame_free(&frameRGB);
                    if (ret < 0)
                        break;
                }
            }
            av_packet_unref(pkt);
            if (ret < 0)
                break;
        }

        sws_freeContext(sws_ctx);
        av_free(buffer);
        av_frame_free(&frameRGB);

        return result;
    }

    virtual Sequence *append(Frame item) {
        cache = PATypes::HashMap<int, double>();
        return PATypes::MutableListSequence<Frame>::append(item);
    }
    static FrameSequence Where(bool (*f)(const Frame &), FrameSequence &input,
                               int windowLength = -1) {
        FrameSequence result(0);
        if (windowLength == -1)
            result = FrameSequence(input.windowLength);
        else
            result = FrameSequence(windowLength);
        auto enumerator = input.getEnumerator();
        while (enumerator->moveNext()) {
            if (f(enumerator->current())) {
                result.append(enumerator->current());
            }
        }
        return result;
    }
    static FrameSequence Where(bool (*f)(const Frame &),
                               FrameSequence &sequence) {
        FrameSequence seq(0);
        auto enumerator = sequence.getEnumerator();
        while (enumerator->moveNext()) {
            if (f(enumerator->current())) {
                seq.append(enumerator->current());
            }
        }
        return seq;
    }
    virtual FrameSequence &Map(Frame (*f)(const Frame &)) {
        auto enumerator = getEnumerator();
        while (enumerator->moveNext()) {
            enumerator->current() = f(enumerator->current());
        }
        return *this;
    }
    void SetWindow(int windowLength) {
        if (windowLength != this->windowLength) {
            this->windowLength = windowLength;
            cache = PATypes::HashMap<int, double>();
        }
    }
    int GetWindow() const { return windowLength; }
    virtual double GetScore(const std::optional<int> &r = std::nullopt) {
        if (r) {
            if (r >= this->getLength()) {
                return 0;
            } else {
                try {
                    return cache.Get(*r);
                } catch (std::out_of_range &e) {
                    cache.Add(*r, GetDeltaScore2(*r) * 1.0);
                    return cache.Get(*r);
                }
            }
        } else {
            return GetDeltaScore2(this->getLength() - 1) * 1.0;
        }
    }
};
} // namespace CCTV