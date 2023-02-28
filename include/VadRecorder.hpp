/*
 ** Copyright 2023-, Qinglong<sysu.zqlong@gmail.com>.
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#include <stdio.h>
#include <stdbool.h>
#include "IAudioEncoder.hpp"

#ifndef __VADRECORDER_H
#define __VADRECORDER_H

class VadRecorderListener {
public:
    VadRecorderListener() : mFile(NULL) {}
    VadRecorderListener(FILE *file) : mFile(file) {}
    virtual ~VadRecorderListener() {}
    /**
     * Callback to output encoded data.
     * \param outBuffer [IN] output buffer pointer.
     * \param outLength [IN] output buffer length in byte. 
     * \retval N/A.
     */
    virtual void onOutputBufferAvailable(unsigned char *outBuffer, int outLength) {
        if (mFile) fwrite(outBuffer, outLength, 1, mFile);
    }
    /**
     * Callback to notify begin of speech.
     * \param N/A.
     * \retval N/A.
     */
    virtual void onSpeechBegin() {}
    /**
     * Callback to notify end of speech.
     * \param N/A.
     * \retval N/A.
     */
    virtual void onSpeechEnd() {}
    /**
     * Callback to notify begin of margin.
     * \param N/A.
     * \retval N/A.
     */
    virtual void onMarginBegin() {}
    /**
     * Callback to notify end of margin.
     * \param N/A.
     * \retval N/A.
     */
    virtual void onMarginEnd() {}
private:
    FILE *mFile;
};

class VadRecorder
{
public:
    enum EncoderType {
        ENCODER_AAC = 0,
        ENCODER_CNT,
    };

    VadRecorder();

    ~VadRecorder();

    void setSpeechMarginMs(unsigned int marginMs) {
        mSpeechMarginMsMax = marginMs > 1000 ? marginMs : 1000;
    }

    static int getPerferredInputBufferSize(int sampleRate, int channels, int bitsPerSample) {
        int frameMs = 30; // valid value for vad engine: { 10ms, 20ms, 30ms }
        int frameSize = sampleRate/1000*frameMs;
        int bufferSize = frameSize*channels*bitsPerSample/8;
        int multiple = 2;
        return bufferSize * multiple;
    }

    bool init(VadRecorderListener *listener,
              int sampleRate, int channels, int bitsPerSample,
              EncoderType encoderType = ENCODER_AAC);

    bool feed(unsigned char *inBuffer, int inLength);

    void deinit();

private:
    VadRecorderListener *mRecorderListener;
    int mSampleRate;
    int mChannels;
    int mBitsPerSample;
    bool mInited;
    EncoderType mEncoderType;
    IAudioEncoder *mEncoderHandle;
    IAudioEncoderListener *mEncoderListener;
    void *mVadHandle;
    bool mSpeechDetected;
    unsigned int mSpeechMarginMsMax;
    unsigned int mSpeechMarginMsVal;
    unsigned char *mCacheBuffer;
    int mCacheBufferLength;
};

#endif // __VADRECORDER_H
