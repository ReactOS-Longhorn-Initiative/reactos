
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

# Replace the old CRT include directory with the UCRT include directory
get_property(INCLUDE_DIRS DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
list(REMOVE_ITEM INCLUDE_DIRS "${REACTOS_SOURCE_DIR}/sdk/include/crt")
set_property(DIRECTORY . PROPERTY INCLUDE_DIRECTORIES ${INCLUDE_DIRS})
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/ucrt)

set(SOURCES
    libavcodec/bsf/null.c
    libavcodec/ac3_parser.c
    libavcodec/adts_parser.c
    libavcodec/allcodecs.c
    libavcodec/avcodec.c
    libavcodec/avdct.c
    libavcodec/packet.c
    libavcodec/bitstream.c
    libavcodec/bitstream_filters.c
    libavcodec/bsf.c
    libavcodec/codec_desc.c
    libavcodec/codec_par.c
    libavcodec/d3d11va.c
    libavcodec/decode.c
    libavcodec/dirac.c
    libavcodec/dv_profile.c
    libavcodec/encode.c
    libavcodec/exif.c
    libavcodec/get_buffer.c
    libavcodec/imgconvert.c
    libavcodec/jni.c
    libavcodec/mathtables.c
    libavcodec/mediacodec.c
    libavcodec/mpeg12framerate.c
    libavcodec/options.c
    libavcodec/parser.c
    libavcodec/parsers.c
    libavcodec/profiles.c
    libavcodec/qsv_api.c
    libavcodec/raw.c
    libavcodec/threadprogress.c
    libavcodec/tiff_common.c
    libavcodec/utils.c
    libavcodec/version.c
    libavcodec/vlc.c
    libavcodec/vorbis_parser.c
    libavcodec/xiph.c
    libavcodec/pthread.c
    libavcodec/pthread_slice.c
    libavcodec/pthread_frame.c
    libavcodec/to_upper4.c

	libavcodec/h264dec.c
	libavcodec/h264_sei.c
	libavcodec/h264_parse.c
	libavcodec/h264_ps.c
	libavcodec/h2645_sei.c
	libavcodec/mpeg4video_parser.c
	libavcodec/golomb.c
	libavcodec/h2645_parse.c
	libavcodec/h263.c
	libavcodec/mpeg4videodec.c
	libavcodec/mpeg4video.c
	libavcodec/ituh263dec.c
	libavcodec/h263data.c
	libavcodec/h2645_vui.c
	libavcodec/h264chroma.c

	libavcodec/h264_cabac.c
	libavcodec/h264_cavlc.c
	libavcodec/h264_direct.c
	libavcodec/h264_loopfilter.c
	libavcodec/h264_mb.c
	libavcodec/h264_picture.c
	libavcodec/h264_refs.c
	libavcodec/h264_slice.c
	libavcodec/h264data.c
	libavcodec/h274.c
	libavcodec/mpegvideo.c
	libavcodec/mpegvideodata.c
	libavcodec/mpegvideo_unquantize.c
	libavcodec/mpegutils.c
	libavcodec/error_resilience.c
	libavcodec/aom_film_grain.c
	libavcodec/h264dsp.c
	libavcodec/h2645data.c
	libavcodec/atsc_a53.c
	libavcodec/cabac.c
	libavcodec/h264pred.c
	libavcodec/rl.c
	libavcodec/me_cmp.c
	libavcodec/h264qpel.c
	libavcodec/x86/videodsp_init.c
	libavcodec/x86/h264dsp_init.c
	libavcodec/videodsp.c
	libavcodec/mpegpicture.c

    libavutil/log.c
    libavutil/log2_tab.c

libavformat/allformats.c
libavformat/av1.c
libavformat/avformat.c
libavformat/avio.c
libavformat/aviobuf.c
libavformat/codecstring.c
libavformat/demux.c
libavformat/demux_utils.c
libavformat/dump.c
libavformat/dv.c
libavformat/format.c
libavformat/id3v1.c
libavformat/id3v2.c
libavformat/isom_tags.c
libavformat/metadata.c
libavformat/mux.c
libavformat/mux_utils.c
libavformat/nal.c
libavformat/options.c
libavformat/os_support.c
libavformat/protocols.c
libavformat/riff.c
libavformat/sdp.c
libavformat/seek.c
libavformat/url.c
libavformat/urldecode.c
libavformat/utils.c
libavformat/version.c
libavformat/vpcc.c

# additional formats
libavformat/h264dec.c
libavformat/av1dec.c
libavformat/rawdec.c

	#libavutil/aarch64/cpu.c
	#libavutil/aarch64/crc.S
	#libavutil/aarch64/float_dsp_init.c
	#libavutil/aarch64/float_dsp_neon.S
	#libavutil/aarch64/tx_float_init.c
	#libavutil/aarch64/tx_float_neon.S
	libavutil/adler32.c
	libavutil/aes.c
	libavutil/aes_ctr.c
	libavutil/ambient_viewing_environment.c
#	libavutil/arm/cpu.c
	#libavutil/arm/float_dsp_init_arm.c
#	libavutil/arm/float_dsp_init_neon.c
#	libavutil/arm/float_dsp_init_vfp.c
#	libavutil/arm/float_dsp_neon.S
#	libavutil/arm/float_dsp_vfp.S
#	libavutil/arm64ec/cpu.c
#	libavutil/arm64ec/crc.S
	libavutil/audio_fifo.c
	libavutil/avsscanf.c
	libavutil/avstring.c
	libavutil/base64.c
	libavutil/blowfish.c
	libavutil/bprint.c
	libavutil/buffer.c
	libavutil/camellia.c
	libavutil/cast5.c
	libavutil/channel_layout.c
	libavutil/container_fifo.c
	libavutil/cpu.c
	libavutil/crc.c
	libavutil/csp.c
	libavutil/des.c
	libavutil/detection_bbox.c
	libavutil/dict.c
	libavutil/display.c
	libavutil/dovi_meta.c
	libavutil/downmix_info.c
	libavutil/encryption_info.c
	libavutil/error.c
	libavutil/eval.c
	libavutil/executor.c
	libavutil/fifo.c
	libavutil/file.c
	libavutil/file_open.c
	libavutil/film_grain_params.c
	libavutil/fixed_dsp.c
	libavutil/float_dsp.c
	libavutil/float_scalarproduct.c
	libavutil/frame.c
	libavutil/half2float.c
	libavutil/hash.c
	libavutil/hdr_dynamic_metadata.c
	libavutil/hdr_dynamic_vivid_metadata.c
	libavutil/hmac.c
	libavutil/hwcontext.c
	libavutil/iamf.c
	libavutil/imgutils.c
	libavutil/integer.c
	libavutil/intmath.c
	libavutil/lfg.c
	libavutil/lls.c
	libavutil/lzo.c
	libavutil/mastering_display_metadata.c
	libavutil/mathematics.c
	libavutil/md5.c
	libavutil/mem.c
	libavutil/murmur3.c
	libavutil/opt.c
	libavutil/parseutils.c
	libavutil/pixdesc.c
	libavutil/pixelutils.c
	libavutil/random_seed.c
	libavutil/rational.c
	libavutil/rc4.c
	libavutil/refstruct.c
	libavutil/reverse.c
	libavutil/ripemd.c
	libavutil/samplefmt.c
	libavutil/sha.c
	libavutil/sha512.c
	libavutil/side_data.c
	libavutil/slicethread.c
	libavutil/spherical.c
	libavutil/stereo3d.c
	libavutil/tdrdi.c
	libavutil/tea.c
	libavutil/threadmessage.c
	libavutil/time.c
	libavutil/timecode.c
	libavutil/timecode_internal.c
	libavutil/timestamp.c
	libavutil/tree.c
	libavutil/twofish.c
	libavutil/tx.c
	libavutil/tx_double.c
	libavutil/tx_float.c
	libavutil/tx_int32.c
	libavutil/utils.c
	libavutil/uuid.c
	libavutil/version.c
	libavutil/video_enc_params.c
	libavutil/video_hint.c
	libavutil/x86/cpu.c
	libavutil/x86/fixed_dsp_init.c
	libavutil/x86/float_dsp_init.c
	libavutil/x86/imgutils_init.c
	libavutil/x86/lls_init.c
	libavutil/x86/tx_float_init.c
	libavutil/xga_font_data.c
	libavutil/xtea.c
	#libswresample/aarch64/audio_convert_init.c
#	libswresample/aarch64/audio_convert_neon.S
#	libswresample/aarch64/resample.S
#	libswresample/aarch64/resample_init.c
#	libswresample/arm/audio_convert_init.c
	#libswresample/arm/audio_convert_neon.S
#	libswresample/arm/resample.S
#	libswresample/arm/resample_init.c
#	libswresample/arm64ec/audio_convert_init.c
#	libswresample/arm64ec/audio_convert_neon.S
#	libswresample/arm64ec/resample.S
#	libswresample/arm64ec/resample_init.c
	libswresample/audioconvert.c
	libswresample/dither.c
	libswresample/options.c
	libswresample/rematrix.c
	libswresample/resample.c
	libswresample/resample_dsp.c
	libswresample/swresample.c
	libswresample/swresample_frame.c
	libswresample/version.c
	libswresample/x86/audio_convert_init.c
	libswresample/x86/rematrix_init.c
	libswresample/x86/resample_init.c
#	libswscale/aarch64/hscale.S
#	libswscale/aarch64/input.S
#	libswscale/aarch64/output.S
#	libswscale/aarch64/range_convert_neon.S
#	libswscale/aarch64/rgb2rgb.c
#	libswscale/aarch64/rgb2rgb_neon.S
##	libswscale/aarch64/swscale.c
#	libswscale/aarch64/swscale_unscaled.c
#	libswscale/aarch64/swscale_unscaled_neon.S
#	libswscale/aarch64/xyz2rgb_neon.S
#	libswscale/aarch64/yuv2rgb_neon.S
	libswscale/alphablend.c
	#libswscale/arm/hscale.S
	#libswscale/arm/output.S
	#libswscale/arm/swscale.c
	#libswscale/arm/swscale_unscaled.c
	#libswscale/arm/yuv2rgb_neon.S
	#libswscale/arm64ec/hscale.S
	#libswscale/arm64ec/input.S
	#libswscale/arm64ec/output.S
	#libswscale/arm64ec/range_convert_neon.S
	#libswscale/arm64ec/rgb2rgb.c
	#libswscale/arm64ec/rgb2rgb_neon.S
	#libswscale/arm64ec/swscale.c
	#libswscale/arm64ec/swscale_unscaled.c
	#libswscale/arm64ec/swscale_unscaled_neon.S
	#libswscale/arm64ec/yuv2rgb_neon.S
	libswscale/cms.c
	libswscale/csputils.c
	libswscale/format.c
	libswscale/gamma.c
	libswscale/graph.c
	libswscale/hscale.c
	libswscale/hscale_fast_bilinear.c
	libswscale/input.c
	libswscale/lut3d.c
	libswscale/ops.c
	libswscale/ops_backend.c
	libswscale/ops_chain.c
	libswscale/ops_dispatch.c
	libswscale/ops_memcpy.c
	libswscale/ops_optimizer.c
	libswscale/options.c
	libswscale/output.c
	libswscale/rgb2rgb.c
	libswscale/slice.c
	libswscale/swscale.c
	libswscale/swscale_unscaled.c
	libswscale/utils.c
	libswscale/version.c
	libswscale/vscale.c
	libswscale/x86/hscale_fast_bilinear_simd.c
	libswscale/x86/ops.c
	libswscale/x86/rgb2rgb.c
	libswscale/x86/swscale.c
	libswscale/x86/yuv2rgb.c
	libswscale/yuv2rgb.c
)


if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64")
    set(FFMPEG_ASSEMBLY_FILES
    
	libavutil/x86/cpuid.asm
	libavutil/x86/crc.asm
	libavutil/x86/fixed_dsp.asm
	libavutil/x86/float_dsp.asm
	libavutil/x86/imgutils.asm
	libavutil/x86/lls.asm
	libavutil/x86/tx_float.asm
	libswresample/x86/audio_convert.asm
	libswresample/x86/rematrix.asm
	libswresample/x86/resample.asm
	libswscale/x86/input.asm
	libswscale/x86/ops_float.asm
	libswscale/x86/ops_int.asm
	libswscale/x86/output.asm
	libswscale/x86/range_convert.asm
	libswscale/x86/rgb_2_rgb.asm
	libswscale/x86/scale.asm
	libswscale/x86/scale_avx2.asm
	libswscale/x86/yuv2yuvX.asm
	libswscale/x86/yuv_2_rgb.asm
    )
endif()

#add_asm_files(ffmpeg_asm ${FFMPEG_ASSEMBLY_FILES})


add_library(ffmpeg STATIC ${SOURCES} ${ffmpeg_asm})
target_compile_definitions(ffmpeg PRIVATE HAVE_AV_CONFIG_H _CRT_DECLARE_NONSTDC_NAMES _CRT_INTERNAL_NONSTDC_NAMES)
add_definitions(-Dcopysign=_copysign)
target_include_directories(ffmpeg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/compat ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/libavcodec ${CMAKE_CURRENT_SOURCE_DIR}/libavutil)
target_include_directories(ffmpeg PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(ffmpeg wine)

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    # error C4312: 'type cast': conversion from 'unsigned long' to 'void *' of greater size
    remove_target_compile_option(ffmpeg "/we4312")

    # error C4113: 'void (__cdecl *)(float *,float *,int)' differs in parameter lists from 'void (__cdecl *)(float *restrict ,float *restrict ,int)'
    # error C4189: 'ignored': local variable is initialized but not referenced
    # error C4101: 't': unreferenced local variable
    # error C4090: 'function': different 'const' qualifiers
    # enable stdatomic.h
    target_compile_options(ffmpeg PUBLIC /wd4113 /wd4189 /wd4101 /wd4090 /experimental:c11atomics)
endif()

# Disable warning as error for third party code
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(ffmpeg PUBLIC -Wno-error -Wno-switch)
endif()
