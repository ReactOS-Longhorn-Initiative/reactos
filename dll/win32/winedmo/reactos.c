/*
 * PROJECT:     ReactOS Media Foundation
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Wrapper around ffmpeg functions to dynamically load ffmpeg from system32
 * COPYRIGHT:   Copyright 2026 Mikhail Tyukin <mishakeys20@gmail.com
 */

#include <windows.h>

#include <stdint.h>
#include <stddef.h>

#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <libavutil/mathematics.h>
#include <libavutil/pixfmt.h>

#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavcodec/codec_id.h>
#include <libavcodec/bsf.h>

#include <libavformat/avformat.h>
#include <libavformat/avio.h>

HMODULE AVUtilLibrary = NULL;
HMODULE AVCodecLibrary = NULL;
HMODULE AVFormatLibrary = NULL;
HMODULE SWScale = NULL;

#define ResolveFFmpegFunction(module, type, name, fail)       \
    do {                                                    \
        if (!(module))                                     \
            return (fail);                                 \
        if (!fn)                                           \
            fn = (type)GetProcAddress((module), #name);    \
        if (!fn)                                           \
            return (fail);                                 \
    } while (0)

#define RESOLVE_FFMPEG_VOID(module, type, name)             \
    do {                                                    \
        if (!(module))                                     \
            return;                                         \
        if (!fn)                                           \
            fn = (type)GetProcAddress((module), #name);    \
        if (!fn)                                           \
            return;                                         \
    } while (0)


BOOL LoadFFmpeg()
{
    BOOL Result = FALSE;

    AVUtilLibrary = LoadLibraryW(L"avutil-60.dll");
    if (!AVUtilLibrary) goto end;

    AVCodecLibrary = LoadLibraryW(L"avcodec-62.dll");
    if (!AVCodecLibrary) goto end;

    AVFormatLibrary = LoadLibraryW(L"avformat-62.dll");
    if (!AVFormatLibrary) goto end;

    SWScale = LoadLibraryW(L"swscale-9.dll");
    if (!SWScale) goto end;

    Result = TRUE;
end:

    if (!Result)
    {
        MessageBoxW(
            GetActiveWindow(), 
            L"FFMpeg is required to view this media.\nPlease install FFmpeg from RAPPS to proceed", 
            L"ReactOS - Media Foundation Error", MB_ICONERROR
        );
    }
    return Result;
}


int av_strerror(int errnum, char *errbuf, size_t errbuf_size)
{
    typedef int (*fn_t)(int, char *, size_t);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(AVUtilLibrary, fn_t, av_strerror, -1);

    return fn(errnum, errbuf, errbuf_size);
}

int64_t av_rescale_q_rnd(
    int64_t a,
    AVRational bq,
    AVRational cq,
    enum AVRounding rnd)
{
    typedef int64_t (*fn_t)(
        int64_t,
        AVRational,
        AVRational,
        enum AVRounding);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(AVUtilLibrary, fn_t, av_rescale_q_rnd, 0);

    return fn(a, bq, cq, rnd);
}

AVDictionaryEntry *av_dict_get(
    const AVDictionary *m,
    const char *key,
    const AVDictionaryEntry *prev,
    int flags)
{
    typedef AVDictionaryEntry *(*fn_t)(
        const AVDictionary *,
        const char *,
        const AVDictionaryEntry *,
        int);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(AVUtilLibrary, fn_t, av_dict_get, NULL);

    return fn(m, key, prev, flags);
}

AVPacket *av_packet_alloc(void)
{
    typedef AVPacket *(*fn_t)(void);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(AVCodecLibrary, fn_t, av_packet_alloc, NULL);

    return fn();
}

void av_packet_free(AVPacket **pkt)
{
    typedef void (*fn_t)(AVPacket **);
    static fn_t fn = NULL;

    RESOLVE_FFMPEG_VOID(AVCodecLibrary, fn_t, av_packet_free);

    fn(pkt);
}

int avcodec_parameters_copy(
    AVCodecParameters *dst,
    const AVCodecParameters *src)
{
    typedef int (*fn_t)(
        AVCodecParameters *,
        const AVCodecParameters *);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVCodecLibrary,
        fn_t,
        avcodec_parameters_copy,
        -1);

    return fn(dst, src);
}

AVIOContext *avio_alloc_context(
    unsigned char *buffer,
    int buffer_size,
    int write_flag,
    void *opaque,
    int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
    int (*write_packet)(void *opaque, const uint8_t *buf, int buf_size),
    int64_t (*seek)(void *opaque, int64_t offset, int whence))
{
    typedef AVIOContext *(*fn_t)(
        unsigned char *,
        int,
        int,
        void *,
        int (*)(void *, uint8_t *, int),
        int (*)(void *, const uint8_t *, int),
        int64_t (*)(void *, int64_t, int));
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVFormatLibrary,
        fn_t,
        avio_alloc_context,
        NULL);

    return fn(
        buffer,
        buffer_size,
        write_flag,
        opaque,
        read_packet,
        write_packet,
        seek);
}

void avio_context_free(AVIOContext **s)
{
    typedef void (*fn_t)(AVIOContext **);
    static fn_t fn = NULL;

    RESOLVE_FFMPEG_VOID(AVFormatLibrary, fn_t, avio_context_free);

    fn(s);
}

AVFormatContext *avformat_alloc_context(void)
{
    typedef AVFormatContext *(*fn_t)(void);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVFormatLibrary,
        fn_t,
        avformat_alloc_context,
        NULL);

    return fn();
}

void avformat_free_context(AVFormatContext *s)
{
    typedef void (*fn_t)(AVFormatContext *);
    static fn_t fn = NULL;

    RESOLVE_FFMPEG_VOID(
        AVFormatLibrary,
        fn_t,
        avformat_free_context);

    fn(s);
}

const AVInputFormat *av_find_input_format(const char *short_name)
{
    typedef const AVInputFormat *(*fn_t)(const char *);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVFormatLibrary,
        fn_t,
        av_find_input_format,
        NULL);

    return fn(short_name);
}

int avformat_open_input(
    AVFormatContext **ps,
    const char *url,
    const AVInputFormat *fmt,
    AVDictionary **options)
{
    typedef int (*fn_t)(
        AVFormatContext **,
        const char *,
        const AVInputFormat *,
        AVDictionary **);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVFormatLibrary,
        fn_t,
        avformat_open_input,
        -1);

    return fn(ps, url, fmt, options);
}

int avformat_find_stream_info(
    AVFormatContext *ic,
    AVDictionary **options)
{
    typedef int (*fn_t)(
        AVFormatContext *,
        AVDictionary **);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVFormatLibrary,
        fn_t,
        avformat_find_stream_info,
        -1);

    return fn(ic, options);
}

int av_read_frame(
    AVFormatContext *s,
    AVPacket *pkt)
{
    typedef int (*fn_t)(AVFormatContext *, AVPacket *);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVFormatLibrary,
        fn_t,
        av_read_frame,
        -1);

    return fn(s, pkt);
}

int avformat_seek_file(
    AVFormatContext *s,
    int stream_index,
    int64_t min_ts,
    int64_t ts,
    int64_t max_ts,
    int flags)
{
    typedef int (*fn_t)(
        AVFormatContext *,
        int,
        int64_t,
        int64_t,
        int64_t,
        int);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVFormatLibrary,
        fn_t,
        avformat_seek_file,
        -1);

    return fn(
        s,
        stream_index,
        min_ts,
        ts,
        max_ts,
        flags);
}

const AVBitStreamFilter *av_bsf_get_by_name(const char *name)
{
    typedef const AVBitStreamFilter *(*fn_t)(const char *);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVCodecLibrary,
        fn_t,
        av_bsf_get_by_name,
        NULL);

    return fn(name);
}

int av_bsf_alloc(
    const AVBitStreamFilter *filter,
    AVBSFContext **ctx)
{
    typedef int (*fn_t)(
        const AVBitStreamFilter *,
        AVBSFContext **);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVCodecLibrary,
        fn_t,
        av_bsf_alloc,
        -1);

    return fn(filter, ctx);
}

int av_bsf_init(AVBSFContext *ctx)
{
    typedef int (*fn_t)(AVBSFContext *);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVCodecLibrary,
        fn_t,
        av_bsf_init,
        -1);

    return fn(ctx);
}

int av_bsf_send_packet(
    AVBSFContext *ctx,
    AVPacket *pkt)
{
    typedef int (*fn_t)(AVBSFContext *, AVPacket *);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVCodecLibrary,
        fn_t,
        av_bsf_send_packet,
        -1);

    return fn(ctx, pkt);
}

int av_bsf_receive_packet(
    AVBSFContext *ctx,
    AVPacket *pkt)
{
    typedef int (*fn_t)(AVBSFContext *, AVPacket *);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVCodecLibrary,
        fn_t,
        av_bsf_receive_packet,
        -1);

    return fn(ctx, pkt);
}

void av_bsf_flush(AVBSFContext *ctx)
{
    typedef void (*fn_t)(AVBSFContext *);
    static fn_t fn = NULL;

    RESOLVE_FFMPEG_VOID(
        AVCodecLibrary,
        fn_t,
        av_bsf_flush);

    fn(ctx);
}

void av_bsf_free(AVBSFContext **ctx)
{
    typedef void (*fn_t)(AVBSFContext **);
    static fn_t fn = NULL;

    RESOLVE_FFMPEG_VOID(
        AVCodecLibrary,
        fn_t,
        av_bsf_free);

    fn(ctx);
}

int av_bsf_get_null_filter(AVBSFContext **bsf)
{
    typedef int (*fn_t)(AVBSFContext **);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVCodecLibrary,
        fn_t,
        av_bsf_get_null_filter,
        -1);

    return fn(bsf);
}

const char *av_get_pix_fmt_name(enum AVPixelFormat pix_fmt)
{
    typedef const char *(*fn_t)(enum AVPixelFormat);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVUtilLibrary,
        fn_t,
        av_get_pix_fmt_name,
        NULL);

    return fn(pix_fmt);
}

const char *avcodec_get_name(enum AVCodecID id)
{
    typedef const char *(*fn_t)(enum AVCodecID);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVCodecLibrary,
        fn_t,
        avcodec_get_name,
        NULL);

    return fn(id);
}

int av_get_bits_per_sample(enum AVCodecID codec_id)
{
    typedef int (*fn_t)(enum AVCodecID);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVCodecLibrary,
        fn_t,
        av_get_bits_per_sample,
        0);

    return fn(codec_id);
}

unsigned int av_codec_get_tag(
    const struct AVCodecTag *const *tags,
    enum AVCodecID id)
{
    typedef unsigned int (*fn_t)(
        const struct AVCodecTag *const *,
        enum AVCodecID);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVFormatLibrary,
        fn_t,
        av_codec_get_tag,
        0);

    return fn(tags, id);
}

const struct AVCodecTag *avformat_get_riff_video_tags(void)
{
    typedef const struct AVCodecTag *(*fn_t)(void);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVFormatLibrary,
        fn_t,
        avformat_get_riff_video_tags,
        NULL);

    return fn();
}

const struct AVCodecTag *avformat_get_riff_audio_tags(void)
{
    typedef const struct AVCodecTag *(*fn_t)(void);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVFormatLibrary,
        fn_t,
        avformat_get_riff_audio_tags,
        NULL);

    return fn();
}

const struct AVCodecTag *avformat_get_mov_video_tags(void)
{
    typedef const struct AVCodecTag *(*fn_t)(void);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVFormatLibrary,
        fn_t,
        avformat_get_mov_video_tags,
        NULL);

    return fn();
}

const struct AVCodecTag *avformat_get_mov_audio_tags(void)
{
    typedef const struct AVCodecTag *(*fn_t)(void);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVFormatLibrary,
        fn_t,
        avformat_get_mov_audio_tags,
        NULL);

    return fn();
}

unsigned avutil_version(void)
{
    typedef unsigned (*fn_t)(void);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVUtilLibrary,
        fn_t,
        avutil_version,
        0);

    return fn();
}

void av_log_set_callback(
    void (*callback)(void *, int, const char *, va_list))
{
    typedef void (*fn_t)(
        void (*)(void *, int, const char *, va_list));

    static fn_t fn = NULL;

    RESOLVE_FFMPEG_VOID(
        AVUtilLibrary,
        fn_t,
        av_log_set_callback);

    fn(callback);
}

unsigned avformat_version(void)
{
    typedef unsigned (*fn_t)(void);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVFormatLibrary,
        fn_t,
        avformat_version,
        0);

    return fn();
}

const AVInputFormat *av_demuxer_iterate(void **opaque)
{
    typedef const AVInputFormat *(*fn_t)(void **);
    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVFormatLibrary,
        fn_t,
        av_demuxer_iterate,
        NULL);

    return fn(opaque);
}

int av_image_copy(
    uint8_t *dst_data[4],
    const int dst_linesizes[4],
    const uint8_t *src_data[4],
    const int src_linesizes[4],
    enum AVPixelFormat pix_fmt,
    int width,
    int height)
{
    typedef int (*fn_t)(
        uint8_t *dst_data[4],
        const int dst_linesizes[4],
        const uint8_t *src_data[4],
        const int src_linesizes[4],
        enum AVPixelFormat pix_fmt,
        int width,
        int height);

    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVUtilLibrary,
        fn_t,
        av_image_copy,
        -1);

    return fn(
        dst_data,
        dst_linesizes,
        src_data,
        src_linesizes,
        pix_fmt,
        width,
        height);
}


int av_image_fill_arrays(
    uint8_t *dst_data[4],
    int dst_linesize[4],
    const uint8_t *src,
    enum AVPixelFormat pix_fmt,
    int width,
    int height,
    int align)
{
    typedef int (*fn_t)(
        uint8_t *dst_data[4],
        int dst_linesize[4],
        const uint8_t *src,
        enum AVPixelFormat pix_fmt,
        int width,
        int height,
        int align);

    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVUtilLibrary,
        fn_t,
        av_image_fill_arrays,
        -1);

    return fn(
        dst_data,
        dst_linesize,
        src,
        pix_fmt,
        width,
        height,
        align);
}


int av_opt_set(
    void *obj,
    const char *name,
    const char *val,
    int search_flags)
{
    typedef int (*fn_t)(
        void *,
        const char *,
        const char *,
        int);

    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVUtilLibrary,
        fn_t,
        av_opt_set,
        -1);

    return fn(obj, name, val, search_flags);
}


int av_opt_set_int(
    void *obj,
    const char *name,
    int64_t val,
    int search_flags)
{
    typedef int (*fn_t)(
        void *,
        const char *,
        int64_t,
        int);

    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVUtilLibrary,
        fn_t,
        av_opt_set_int,
        -1);

    return fn(obj, name, val, search_flags);
}


int av_opt_set_pixel_fmt(
    void *obj,
    const char *name,
    enum AVPixelFormat fmt,
    int search_flags)
{
    typedef int (*fn_t)(
        void *,
        const char *,
        enum AVPixelFormat,
        int);

    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVUtilLibrary,
        fn_t,
        av_opt_set_pixel_fmt,
        -1);

    return fn(obj, name, fmt, search_flags);
}


AVBufferRef *av_buffer_create(
    uint8_t *data,
    size_t size,
    void (*free)(void *opaque, uint8_t *data),
    void *opaque,
    int flags)
{
    typedef AVBufferRef *(*fn_t)(
        uint8_t *,
        size_t,
        void (*)(void *, uint8_t *),
        void *,
        int);

    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVUtilLibrary,
        fn_t,
        av_buffer_create,
        NULL);

    return fn(
        data,
        size,
        free,
        opaque,
        flags);
}


AVBufferRef *av_buffer_ref(const AVBufferRef *buf)
{
    typedef AVBufferRef *(*fn_t)(const AVBufferRef *);

    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVUtilLibrary,
        fn_t,
        av_buffer_ref,
        NULL);

    return fn(buf);
}


void av_buffer_unref(AVBufferRef **buf)
{
    typedef void (*fn_t)(AVBufferRef **);

    static fn_t fn = NULL;

    RESOLVE_FFMPEG_VOID(
        AVUtilLibrary,
        fn_t,
        av_buffer_unref);

    fn(buf);
}


void av_frame_unref(AVFrame *frame)
{
    typedef void (*fn_t)(AVFrame *);

    static fn_t fn = NULL;

    RESOLVE_FFMPEG_VOID(
        AVUtilLibrary,
        fn_t,
        av_frame_unref);

    fn(frame);
}


void av_frame_move_ref(
    AVFrame *dst,
    AVFrame *src)
{
    typedef void (*fn_t)(
        AVFrame *,
        AVFrame *);

    static fn_t fn = NULL;

    RESOLVE_FFMPEG_VOID(
        AVUtilLibrary,
        fn_t,
        av_frame_move_ref);

    fn(dst, src);
}


int av_frame_get_buffer(
    AVFrame *frame,
    int align)
{
    typedef int (*fn_t)(
        AVFrame *,
        int);

    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        AVUtilLibrary,
        fn_t,
        av_frame_get_buffer,
        -1);

    return fn(frame, align);
}

unsigned swscale_version(void)
{
    typedef unsigned (*fn_t)(void);

    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        SWScale,
        fn_t,
        swscale_version,
        0);

    return fn();
}


struct SwsContext *sws_alloc_context(void)
{
    typedef struct SwsContext *(*fn_t)(void);

    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        SWScale,
        fn_t,
        sws_alloc_context,
        NULL);

    return fn();
}


void sws_free_context(struct SwsContext *swsContext)
{
    typedef void (*fn_t)(struct SwsContext *);

    static fn_t fn = NULL;

    RESOLVE_FFMPEG_VOID(
        SWScale,
        fn_t,
        sws_free_context);

    fn(swsContext);
}


int sws_init_context(
    struct SwsContext *sws_context,
    struct SwsFilter *srcFilter,
    struct SwsFilter *dstFilter)
{
    typedef int (*fn_t)(
        struct SwsContext *,
        struct SwsFilter *,
        struct SwsFilter *);

    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        SWScale,
        fn_t,
        sws_init_context,
        -1);

    return fn(
        sws_context,
        srcFilter,
        dstFilter);
}


int sws_scale_frame(
    struct SwsContext *c,
    struct AVFrame *dst,
    const AVFrame *src)
{
    typedef int (*fn_t)(
        struct SwsContext *,
        struct AVFrame *,
        const AVFrame *);

    static fn_t fn = NULL;

    ResolveFFmpegFunction(
        SWScale,
        fn_t,
        sws_scale_frame,
        -1);

    return fn(c, dst, src);
}
