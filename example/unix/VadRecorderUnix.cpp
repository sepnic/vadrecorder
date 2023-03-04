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
#include <string.h>
#include <time.h>
#include "portaudio.h"
#include "VadRecorder.hpp"
#include "logger.h"

#define TAG "VadRecorderUnix"

#define SAMPLE_RATE      16000
#define CHANNEL_COUNT    1
#define SAMPLE_BITS      16
#define SPEECH_MARGIN    (5*1000) // in ms

class VadRecorderListenerUnix : public VadRecorderListener
{
public:
    VadRecorderListenerUnix(FILE *voiceFile, FILE *timestampFile)
      : mVoiceFile(voiceFile), mTimestampFile(timestampFile) {}
    ~VadRecorderListenerUnix() {}
    void onOutputBufferAvailable(char *outBuffer, int outLength) { updateVoiceFile(outBuffer, outLength); }
    void onSpeechBegin() { updateTimestampFile("Speech Begin"); }
    void onSpeechEnd() { updateTimestampFile("Speech End"); }
    void onMarginBegin() { updateTimestampFile("Margin Begin"); }
    void onMarginEnd() { updateTimestampFile("Margin End"); }
private:
    FILE *mVoiceFile;
    FILE *mTimestampFile;
    void updateVoiceFile(char *outBuffer, int outLength) {
        if (mVoiceFile != NULL)
            fwrite(outBuffer, outLength, 1, mVoiceFile);
    }
    void updateTimestampFile(const char *event) {
        char buffer[128];
        struct tm now;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        localtime_r(&ts.tv_sec, &now);
        memset(buffer, 0x0, sizeof(buffer));
        int ret = snprintf(buffer, sizeof(buffer),
                "%4d-%02d-%02d %02d:%02d:%02d:%03d [%s]\n",
                now.tm_year+1900, now.tm_mon+1, now.tm_mday,
                now.tm_hour, now.tm_min, now.tm_sec, (int)((ts.tv_nsec/1000000)%1000),
                event);
        if (mTimestampFile != NULL)
            fwrite(buffer, ret, 1, mTimestampFile);
    }
};

static int PortAudio_InStreamCallback(const void *input, void *output,
    unsigned long frame_count, const PaStreamCallbackTimeInfo *time_info,
    PaStreamCallbackFlags status_flags, void *user_data)
{
    VadRecorder *vadRecorder = static_cast<VadRecorder *>(user_data);
    unsigned long nbytes = frame_count*CHANNEL_COUNT*SAMPLE_BITS/8;
    if (!vadRecorder->feed((char *)input, nbytes))
        pr_err("Failed to feed pcm data to VadRecorder");
    return paContinue;
}

int main(int argc, char *argv[])
{
    const char *voiceFileName = "rec_voice.aac";
    const char *timestampFileName = "rec_timestamp.txt";
    if (argc < 3) {
        pr_wrn("Usage: %s rec_voice.aac rec_timestamp.txt", argv[0]);
    } else {
        voiceFileName = argv[1];
        timestampFileName = argv[2];
    }

    FILE *voiceFile = fopen(voiceFileName, "wb");
    if (voiceFile == NULL) {
        pr_err("Failed to open output aac file: %s", voiceFileName);
        return -1;
    }

    FILE *timestampFile = fopen(timestampFileName, "w");
    if (timestampFile == NULL) {
        pr_err("Failed to open output timestamp file: %s", timestampFileName);
        return -1;
    }

    // VadRecorder init
    VadRecorder *vadRecorder = new VadRecorder();
    VadRecorderListener *vadRecorderListener =
            new VadRecorderListenerUnix(voiceFile, timestampFile);
    vadRecorder->setSpeechMarginMs(SPEECH_MARGIN);
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
    fclose(voiceFile);
    fclose(timestampFile);
    return 0;
}
