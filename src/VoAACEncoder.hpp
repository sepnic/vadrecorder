/*
 ** Copyright 2019-2023, Qinglong<sysu.zqlong@gmail.com>.
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

#ifndef __VOAACENCODER_H
#define __VOAACENCODER_H

#include "voAAC.h"
#include "IAudioEncoder.hpp"

class VoAACEncoder : public IAudioEncoder
{
public:
    VoAACEncoder();

    ~VoAACEncoder();

    int init(IAudioEncoderListener *listener,
             int sampleRate, int channels, int bitsPerSample);

    int encode(unsigned char *inBuffer, int inLength);

    void deinit();

private:
    IAudioEncoderListener  *mListener;
    VO_AUDIO_CODECAPI      mCodecApi;
    VO_MEM_OPERATOR        mCodecMem;
    VO_HANDLE              mCodecHandle;
    VO_CODECBUFFER         mOutBuffer;
    VO_CODECBUFFER         mInBuffer;
    int                    mBytesRemain;
    int                    mBytesFrame;
    int preferredBitRate(int sampleRate, int channels);
};

#endif // __VOAACENCODER_H
