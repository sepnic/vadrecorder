// Copyright (c) 2023-, Qinglong<sysu.zqlong@gmail.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "portaudio.h"
#include "VadRecorder.hpp"
#include "logger.h"

#define TAG "VadRecorderUnix"

#define SAMPLE_RATE      16000
#define CHANNEL_COUNT    1
#define SAMPLE_BITS      16
#define VOICE_MARGIN     (3*1000) // in ms

static int PortAudio_InStreamCallback(const void *input, void *output,
    unsigned long frame_count, const PaStreamCallbackTimeInfo *time_info,
    PaStreamCallbackFlags status_flags, void *user_data)
{
    VadRecorder *vadRecorder = static_cast<VadRecorder *>(user_data);
    unsigned long nbytes = frame_count*CHANNEL_COUNT*SAMPLE_BITS/8;
    if (!vadRecorder->feed((unsigned char *)input, nbytes))
        pr_err("Failed to feed pcm data to VadRecorder");
    return paContinue;
}

int main(int argc, char *argv[])
{
    const char *filename = "out.aac";
    if (argc < 2)
        pr_wrn("Usage: %s out.aac", argv[0]);
    else
        filename = argv[1];

    FILE *outFile = fopen(filename, "wb");
    if (outFile == NULL) {
        pr_err("Failed to open output file");
        return -1;
    }

    // VadRecorder init
    VadRecorder *vadRecorder = new VadRecorder();
    VadRecorderListener *vadRecorderListener = new VadRecorderListener(outFile);
    vadRecorder->setVoiceMarginMs(VOICE_MARGIN);
    if (!vadRecorder->init(vadRecorderListener, SAMPLE_RATE, CHANNEL_COUNT, SAMPLE_BITS)) {
        pr_err("Failed to init VadRecorder");
        goto __out;
    }

    // PortAudio init
    {
        if (Pa_Initialize() != paNoError) {
            pr_err("Failed to Pa_Initialize");
            goto __out;
        }
        int perferredFrameSize = VadRecorder::getPerferredInputBufferSize(
                SAMPLE_RATE, CHANNEL_COUNT, SAMPLE_BITS);
        unsigned long framesNeed = perferredFrameSize/(CHANNEL_COUNT*SAMPLE_BITS/8);
        PaStream *inStream = NULL;
        PaStreamParameters inParameters;
        inParameters.device = Pa_GetDefaultInputDevice();
        if (inParameters.device == paNoDevice) {
            pr_err("Failed to Pa_GetDefaultInputDevice");
            goto __out;
        }
        inParameters.channelCount = CHANNEL_COUNT;
        switch (SAMPLE_BITS) {
        case 32:
            inParameters.sampleFormat = paInt32;
            break;
        case 24:
            inParameters.sampleFormat = paInt24;
            break;
        case 16:
        default:
            inParameters.sampleFormat = paInt16;
            break;
        }
        inParameters.suggestedLatency =
                Pa_GetDeviceInfo(inParameters.device)->defaultLowInputLatency;
        inParameters.hostApiSpecificStreamInfo = NULL;
        if (Pa_OpenStream(&inStream, &inParameters, NULL,
                SAMPLE_RATE, framesNeed, paFramesPerBufferUnspecified,
                PortAudio_InStreamCallback, (void *)vadRecorder) != paNoError) {
            pr_err("Failed to Pa_OpenStream");
            goto __out;
        }
        if (Pa_StartStream(inStream) != paNoError) {
            pr_err("Failed to Pa_StartStream");
            goto __out;
        }
    }

    while (1) {
        pr_wrn("Waiting key event:");
        pr_wrn("  Q|q   : quit");
        char keyevent = getc(stdin);
        if (keyevent == 'Q' || keyevent == 'q') {
            pr_wrn("Quit");
            break;
        }
    }

__out:
    Pa_Terminate();
    delete vadRecorder;
    delete vadRecorderListener;
    fclose(outFile);
    return 0;
}
