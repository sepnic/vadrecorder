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
#include "lockfree_ringbuf.h"
#include "VadRecorder.hpp"

#define TAG "VadRecorder"

static const int kCacheTimeInMs = 1000;

class VoAACEncoderListener : public IAudioEncoderListener
{
public:
    VoAACEncoderListener(VadRecorderListener *listener)
        : mRecorderListener(listener) {}
    void onOutputBufferAvailable(char *outBuffer, int outLength) {
        mRecorderListener->onOutputBufferAvailable(outBuffer, outLength);
    }
private:
    VadRecorderListener *mRecorderListener;
};

VadRecorder::VadRecorder()
    : mInited(false),
      mRecorderListener(NULL),
      mEncoderType(ENCODER_AAC),
      mEncoderHandle(NULL),
      mEncoderListener(NULL),
      mVadHandle(NULL),
      mSpeechDetected(false),
      mSpeechMarginMsMax(0),
      mSpeechMarginMsVal(0),
      mInputBuffer(NULL),
      mInputBufferRemain(0),
      mCacheRingbuf(NULL),
      mCacheBuffer(NULL),
      mCacheBufferSize(0)
{}

VadRecorder::~VadRecorder()
{
    if (mVadHandle != NULL)
        litevad_destroy(mVadHandle);
    if (mEncoderHandle != NULL)
        delete mEncoderHandle;
    if (mEncoderListener != NULL)
        delete mEncoderListener;
    if (mInputBuffer != NULL)
        delete [] mInputBuffer;
    if (mCacheBuffer != NULL)
        delete [] mCacheBuffer;
    if (mCacheRingbuf != NULL)
        lockfree_ringbuf_destroy(mCacheRingbuf);
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

    int frameCountPer10Ms = sampleRate/100;
    int frameBytesPer10Ms = frameCountPer10Ms*channels*bitsPerSample/8;

    mInputBufferRemain = 0;
    mInputBufferSize = frameBytesPer10Ms*3;
    mInputBuffer = new char[mInputBufferSize];

    mCacheBufferSize = frameBytesPer10Ms*10; // cache buffer for 100ms data
    mCacheBuffer = new char[mCacheBufferSize];
    mCacheRingbuf = lockfree_ringbuf_create(frameBytesPer10Ms*kCacheTimeInMs/10);
    if (mCacheRingbuf == NULL) {
        pr_err("Failed to allocate cache buffer");
        return false;
    }

    mVadHandle = litevad_create(sampleRate, channels, bitsPerSample);
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

bool VadRecorder::process(char *inBuffer, int inLength)
{
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
        int cacheSize = lockfree_ringbuf_bytes_filled(mCacheRingbuf);
        pr_dbg("Encode cache buffer: size:%d", cacheSize);
        while (cacheSize > 0) {
            int readSize = lockfree_ringbuf_read(mCacheRingbuf, mCacheBuffer, mCacheBufferSize);
            if (readSize > 0) {
                mEncoderHandle->encode(mCacheBuffer, readSize);
                cacheSize -= readSize;
            } else {
                break;
            }
        }
        pr_dbg("Encode intput buffer: size:%d", inLength);
        return mEncoderHandle->encode(inBuffer, inLength) == IAudioEncoder::ENCODER_NOERROR;
    } else {
        lockfree_ringbuf_unsafe_overwrite(mCacheRingbuf, inBuffer, inLength);
        return true;
    }
}

bool VadRecorder::feed(char *inBuffer, int inLength)
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

    int frameCountPer10Ms = mSampleRate/100;
    int frameBytesPer10Ms = frameCountPer10Ms*mChannels*mBitsPerSample/8;

    if (mInputBufferRemain > 0) {
        if (mInputBufferSize >= frameBytesPer10Ms && frameBytesPer10Ms > mInputBufferRemain) {
            int filledSize = frameBytesPer10Ms-mInputBufferRemain;
            if (filledSize <= inLength) {
                memcpy(&mInputBuffer[mInputBufferRemain], inBuffer, filledSize);
                inBuffer += filledSize;
                inLength -= filledSize;
                mInputBufferRemain = 0;
                if (!process(mInputBuffer, frameBytesPer10Ms))
                    return false;
            }
        } else {
            pr_err("Input buffer size is too small, it should not happen");
            mInputBufferRemain = 0;
            return false;
        }
    }

    while (inLength >= frameBytesPer10Ms) {
        if (!process(inBuffer, frameBytesPer10Ms)) {
            mInputBufferRemain = 0;
            return false;
        }
        inBuffer += frameBytesPer10Ms;
        inLength -= frameBytesPer10Ms;
    }

    if (inLength+mInputBufferRemain <= mInputBufferSize) {
        memcpy(&mInputBuffer[mInputBufferRemain], inBuffer, inLength);
        mInputBufferRemain += inLength;
    } else {
        pr_err("Input buffer size is too small, it should not happen");
        mInputBufferRemain = 0;
        return false;
    }

    return true;;
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
        delete [] mInputBuffer;
        mInputBuffer = NULL;
        delete [] mCacheBuffer;
        mCacheBuffer = NULL;
        lockfree_ringbuf_destroy(mCacheRingbuf);
        mCacheRingbuf = NULL;
        mInited = false;
    }
}

