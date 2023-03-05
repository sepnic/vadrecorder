cmake_minimum_required(VERSION 2.8)

project(vadrecorder)

# cflags
set(CMAKE_C_FLAGS   "-Wall -Werror -std=gnu11")
set(CMAKE_CXX_FLAGS "-Wall -Werror -std=c++11")

set(TOP_DIR         "${CMAKE_SOURCE_DIR}/../..")
set(VOAAC_DIR       "${TOP_DIR}/thirdparty/aacenc")
set(WEBRTC_DIR      "${TOP_DIR}/thirdparty/webrtc")
set(VADREC_DIR      "${TOP_DIR}/src")

# include files
include_directories(${TOP_DIR}/include)
include_directories(${VOAAC_DIR}/include)
include_directories(${WEBRTC_DIR}/inc)

# VOAAC_SRC source files
set (VOAAC_SRC
    ${VOAAC_DIR}/common/cmnMemory.c
    ${VOAAC_DIR}/aacenc/basic_op/basicop2.c
    ${VOAAC_DIR}/aacenc/basic_op/oper_32b.c
    ${VOAAC_DIR}/aacenc/src/aac_rom.c
    ${VOAAC_DIR}/aacenc/src/aacenc.c
    ${VOAAC_DIR}/aacenc/src/aacenc_core.c
    ${VOAAC_DIR}/aacenc/src/adj_thr.c
    ${VOAAC_DIR}/aacenc/src/band_nrg.c
    ${VOAAC_DIR}/aacenc/src/bit_cnt.c
    ${VOAAC_DIR}/aacenc/src/bitbuffer.c
    ${VOAAC_DIR}/aacenc/src/bitenc.c
    ${VOAAC_DIR}/aacenc/src/block_switch.c
    ${VOAAC_DIR}/aacenc/src/channel_map.c
    ${VOAAC_DIR}/aacenc/src/dyn_bits.c
    ${VOAAC_DIR}/aacenc/src/grp_data.c
    ${VOAAC_DIR}/aacenc/src/interface.c
    ${VOAAC_DIR}/aacenc/src/line_pe.c
    ${VOAAC_DIR}/aacenc/src/ms_stereo.c
    ${VOAAC_DIR}/aacenc/src/pre_echo_control.c
    ${VOAAC_DIR}/aacenc/src/psy_configuration.c
    ${VOAAC_DIR}/aacenc/src/psy_main.c
    ${VOAAC_DIR}/aacenc/src/qc_main.c
    ${VOAAC_DIR}/aacenc/src/quantize.c
    ${VOAAC_DIR}/aacenc/src/sf_estim.c
    ${VOAAC_DIR}/aacenc/src/spreading.c
    ${VOAAC_DIR}/aacenc/src/stat_bits.c
    ${VOAAC_DIR}/aacenc/src/tns.c
    ${VOAAC_DIR}/aacenc/src/transform.c
    ${VOAAC_DIR}/aacenc/src/memalign.c)

# WEBRTC_SRC source files
set(WEBRTC_SRC
    ${WEBRTC_DIR}/src/signal_processing/energy.c
    ${WEBRTC_DIR}/src/signal_processing/division_operations.c
    ${WEBRTC_DIR}/src/signal_processing/get_scaling_square.c
    ${WEBRTC_DIR}/src/signal_processing/resample.c
    ${WEBRTC_DIR}/src/signal_processing/resample_48khz.c
    ${WEBRTC_DIR}/src/signal_processing/resample_by_2.c
    ${WEBRTC_DIR}/src/signal_processing/resample_by_2_internal.c
    ${WEBRTC_DIR}/src/signal_processing/resample_fractional.c
    ${WEBRTC_DIR}/src/vad/vad_core.c
    ${WEBRTC_DIR}/src/vad/vad_filterbank.c
    ${WEBRTC_DIR}/src/vad/vad_gmm.c
    ${WEBRTC_DIR}/src/vad/vad_sp.c
    ${WEBRTC_DIR}/src/vad/webrtc_vad.c
    ${WEBRTC_DIR}/src/resampler/resampler.cc)

# VADRECORDER_SRC source files
set(VADREC_SRC
    ${VADREC_DIR}/VadRecorder.cpp
    ${VADREC_DIR}/VoAACEncoder.cpp
    ${VADREC_DIR}/litevad.c
    ${VADREC_DIR}/lockfree_ringbuf.c)

# libvadrecorder
add_library(vadrecorder STATIC ${VADREC_SRC} ${VOAAC_SRC} ${WEBRTC_SRC})

###############################################################################
## portaudio
set(PORTAUDIO_DIR   "${TOP_DIR}/thirdparty/portaudio")
include_directories(${PORTAUDIO_DIR}/include)
include_directories(${PORTAUDIO_DIR}/src/common)
include_directories(${PORTAUDIO_DIR}/src/os/unix)
set(PORTAUDIO_SRC
    ${PORTAUDIO_DIR}/src/common/pa_allocation.c
    ${PORTAUDIO_DIR}/src/common/pa_converters.c
    ${PORTAUDIO_DIR}/src/common/pa_cpuload.c
    ${PORTAUDIO_DIR}/src/common/pa_debugprint.c
    ${PORTAUDIO_DIR}/src/common/pa_dither.c
    ${PORTAUDIO_DIR}/src/common/pa_front.c
    ${PORTAUDIO_DIR}/src/common/pa_process.c
    ${PORTAUDIO_DIR}/src/common/pa_ringbuffer.c
    ${PORTAUDIO_DIR}/src/common/pa_stream.c
    ${PORTAUDIO_DIR}/src/common/pa_trace.c
    ${PORTAUDIO_DIR}/src/hostapi/skeleton/pa_hostapi_skeleton.c
    ${PORTAUDIO_DIR}/src/os/unix/pa_unix_hostapis.c
    ${PORTAUDIO_DIR}/src/os/unix/pa_unix_util.c)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPA_USE_ALSA=1")
    set(PORTAUDIO_SRC ${PORTAUDIO_SRC} ${PORTAUDIO_DIR}/src/hostapi/alsa/pa_linux_alsa.c)
    set(PLATFORM_LIB "asound")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPA_USE_COREAUDIO=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Accelerate")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework CoreAudio")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework AudioToolbox")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework AudioUnit")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework CoreServices")
    set(PORTAUDIO_SRC ${PORTAUDIO_SRC}
        ${PORTAUDIO_DIR}/src/hostapi/coreaudio/pa_mac_core.c
        ${PORTAUDIO_DIR}/src/hostapi/coreaudio/pa_mac_core_blocking.c
        ${PORTAUDIO_DIR}/src/hostapi/coreaudio/pa_mac_core_utilities.c)
endif()
include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPA_BIG_ENDIAN")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPA_LITTLE_ENDIAN")
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=deprecated-declarations")
## example
add_executable(VadRecorderUnix ${CMAKE_SOURCE_DIR}/VadRecorderUnix.cpp ${PORTAUDIO_SRC})
target_link_libraries(VadRecorderUnix vadrecorder pthread ${PLATFORM_LIB})
###############################################################################
