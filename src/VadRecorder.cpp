/*
 ** Copyright 2023-, LUOYUN <sysu.zqlong@gmail.com>.
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

#include <string.h>
#include "logger.h"
#include "litevad.h"
#include "IAudioEncoder.hpp"
#include "VoAACEncoder.hpp"
#include "VadRecorder.hpp"

#define TAG "VadRecorder"

class VoAACEncoderListener : public IAudioEncoderListener
{
public:
    VoAACEncoderListener(VadRecorderListener *listener)
        : mRecorderListener(listener) {}
    void onOutputBufferAvailable(unsigned char *outBuffer, int outLength) {
        mRecorderListener->onOutputBufferAvailable(outBuffer, outLength);
    }
private:
    VadRecorderListener *mRecorderListener;
};

VadRecorder::VadRecorder()
    : mRecorderListener(NULL),
      mInited(false),
      mEncoderType(ENCODER_AAC),
      mEncoderHandle(NULL),
      mEncoderListener(NULL),
      mVadHandle(NULL),
      mSpeechDetected(false),
      mSpeechMarginMsMax(0),
      mSpeechMarginMsVal(0),
      mCacheBuffer(NULL),
      mCacheBufferLength(0)
{}

VadRecorder::~VadRecorder()
{
    if (mVadHandle != NULL)
        litevad_destroy(mVadHandle);
    if (mEncoderHandle != NULL)
        delete mEncoderHandle;
    if (mEncoderListener != NULL)
        delete mEncoderListener;
    if (mCacheBuffer)
        delete [] mCacheBuffer;
}

bool VadRecorder::init(VadRecorderListener *listener,
            int sampleRate, int channels, int bitsPerSample,
            EncoderType encoderType)
{
    pr_dbg("Init VadRecorder:%dHz/%dCh/%dBits, codec:%d",
           sampleRate, channels, bitsPerSample, encoderType);
    if (mInited) {
        pr_wrn("Reconfig VadRecorder");
        deinit();
    }

    if (listener == NULL) {
        pr_err("Invalid recorder listener");
        return false;
    }

    mVadHandle = litevad_create(sampleRate, channels);
    if (mVadHandle == NULL) {
        pr_err("Failed to litevad_create");
        return false;
    }

    switch (encoderType) {
    case ENCODER_AAC:
        mEncoderHandle = new VoAACEncoder();
        mEncoderListener = new VoAACEncoderListener(listener);
        break;
    default:
        pr_err("Invalid encoder type, only aac supported");
        return false;
    }

    int ret = mEncoderHandle->init(mEncoderListener, sampleRate, channels, bitsPerSample);
    if (ret != IAudioEncoder::ENCODER_NOERROR) {
        pr_err("Failed to init audio encoder");
        return false;
    }

    mRecorderListener = listener;
    mEncoderType = encoderType;
    mSpeechDetected = false;
    mSpeechMarginMsVal = 0;
    mSampleRate = sampleRate;
    mChannels = channels;
    mBitsPerSample = bitsPerSample;
    mInited = true;
    return mInited;
}

bool VadRecorder::feed(unsigned char *inBuffer, int inLength)
{
    pr_dbg("Feed input buffer:%p, size:%d", inBuffer, inLength);
    if (!mInited) {
        pr_err("VadRecorder not inited");
        return false;
    }

    if (inBuffer == NULL || inLength <= 0) {
        pr_err("Invalid input buffer or input length");
        return false;
    }

    litevad_result_t vadResult = litevad_process(mVadHandle, inBuffer, inLength);
    switch (vadResult) {
    case LITEVAD_RESULT_SPEECH_BEGIN:
        mSpeechDetected = true;
        mRecorderListener->onSpeechBegin();
        break;
    case LITEVAD_RESULT_SPEECH_END:
        mSpeechDetected = false;
        mSpeechMarginMsVal = 0;
        mRecorderListener->onSpeechEnd();
        if (mSpeechMarginMsMax > 0)
            mRecorderListener->onMarginBegin();
        break;
    case LITEVAD_RESULT_ERROR:
        pr_err("Invalid litevad result");
        return false;
    default:
        break;
    }

    bool needEncode = mSpeechDetected;
    if (!mSpeechDetected) {
        if (mSpeechMarginMsMax > 0 && mSpeechMarginMsVal <= mSpeechMarginMsMax) {
            needEncode = true;
            mSpeechMarginMsVal += inLength*1000/(mSampleRate*mChannels*mBitsPerSample/8);
            if (mSpeechMarginMsVal > mSpeechMarginMsMax)
                mRecorderListener->onMarginEnd();
        }
    }

    if (needEncode) {
        if (mCacheBufferLength > 0) {
            pr_dbg("Encode cache buffer:%p, size:%d", mCacheBuffer, mCacheBufferLength);
            mEncoderHandle->encode(mCacheBuffer, mCacheBufferLength);
            mCacheBufferLength = 0;
        }
        return mEncoderHandle->encode(inBuffer, inLength) == IAudioEncoder::ENCODER_NOERROR;
    } else {
        if (mCacheBuffer == NULL) {
            mCacheBuffer = new unsigned char[inLength];
        } else if (mCacheBufferLength < inLength) {
            delete [] mCacheBuffer;
            mCacheBuffer = new unsigned char[inLength];
        }
        memcpy(mCacheBuffer, inBuffer, inLength);
        mCacheBufferLength = inLength;
        return true;
    }
}

void VadRecorder::deinit()
{
    pr_dbg("Deinit VadRecorder");
    if (mInited) {
        litevad_destroy(mVadHandle);
        mVadHandle = NULL;
        mEncoderHandle->deinit();
        delete mEncoderHandle;
        mEncoderHandle = NULL;
        delete mEncoderListener;
        mEncoderListener = NULL;
        mInited = false;
    }
}

