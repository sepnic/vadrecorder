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
target_include_directories(vadrecorder PRIVATE ${VOAAC_DIR}/include ${WEBRTC_DIR}/inc)

###############################################################################
## portaudio
execute_process(COMMAND ${CMAKE_SOURCE_DIR}/install_portaudio.sh
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/portaudio/install/include)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/portaudio/install/lib)
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Accelerate")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework CoreAudio")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework AudioToolbox")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework AudioUnit")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework CoreServices")
endif()
## example
add_executable(VadRecorderUnix ${CMAKE_SOURCE_DIR}/VadRecorderUnix.cpp)
target_link_libraries(VadRecorderUnix vadrecorder portaudio pthread)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(VadRecorderUnix asound)
endif()
###############################################################################
