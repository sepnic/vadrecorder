/*
 * Copyright (C) 2023- Qinglong<sysu.zqlong@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <jni.h>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <cassert>
#include "VadRecorder.hpp"
#include "logger.h"

#define TAG "VadRecorderJNI"

#define JAVA_CLASS_NAME "com/example/vadrecorder_demo/MainActivity"
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))

static const int kSpeechMarginMs = 3000; // in ms

class VadRecorderListenerJni : public VadRecorderListener
{
public:
    VadRecorderListenerJni() : mVoiceFile(nullptr), mTimestampFile(nullptr) {}
    ~VadRecorderListenerJni() {}
    void setVoiceFile(FILE *file) { mVoiceFile = file; }
    void setTimestampFile(FILE *file) { mTimestampFile = file; }
    void onOutputBufferAvailable(char *outBuffer, int outLength) { updateVoiceFile(outBuffer, outLength); }
    void onSpeechBegin() { updateTimestampFile("Speech Begin"); }
    void onSpeechEnd() { updateTimestampFile("Speech End"); }
    void onMarginBegin() { updateTimestampFile("Margin Begin"); }
    void onMarginEnd() { updateTimestampFile("Margin End"); }
private:
    FILE *mVoiceFile;
    FILE *mTimestampFile;
    void updateVoiceFile(char *outBuffer, int outLength) {
        if (mVoiceFile != nullptr)
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
        if (mTimestampFile != nullptr)
            fwrite(buffer, ret, 1, mTimestampFile);
    }
};

class VadRecorderWrapperJni {
public:
    VadRecorderWrapperJni() : mRecordVoiceFile(nullptr), mRecordTimestampFile(nullptr) {
        mVadRecorder = new VadRecorder();
        mVadRecorderListener = new VadRecorderListenerJni();
        mVadRecorder->setSpeechMarginMs(kSpeechMarginMs);
    }
    ~VadRecorderWrapperJni() {
        delete mVadRecorder;
        delete mVadRecorderListener;
        if (mRecordVoiceFile != nullptr)
            fclose(mRecordVoiceFile);
        if (mRecordTimestampFile != nullptr)
            fclose(mRecordTimestampFile);
    }
    VadRecorder *mVadRecorder;
    VadRecorderListenerJni *mVadRecorderListener;
    FILE *mRecordVoiceFile;
    FILE *mRecordTimestampFile;
};

static void jniThrowException(JNIEnv *env, const char *className, const char *msg) {
    jclass clazz = env->FindClass(className);
    if (!clazz) {
        pr_err("Unable to find exception class %s", className);
        /* ClassNotFoundException now pending */
        return;
    }

    if (env->ThrowNew(clazz, msg) != JNI_OK) {
        pr_err("Failed throwing '%s' '%s'", className, msg);
        /* an exception, most likely OOM, will now be pending */
    }
    env->DeleteLocalRef(clazz);
}

static jlong VadRecorder_Create(JNIEnv* env, jobject thiz)
{
    pr_dbg("VadRecorderCreate");
    auto *recorderWrapper = new VadRecorderWrapperJni();
    return (jlong)recorderWrapper;
}

static jboolean VadRecorder_Init(JNIEnv *env, jobject thiz, jlong handle,
                                jstring voiceFilePath, jstring timestampFilePath,
                                jint sampleRate, jint channels, jint bitsPerSample)
{
    pr_dbg("VadRecorderInit: %dHz/%dCh/%dBits", sampleRate, channels, bitsPerSample);
    auto recorderWrapper = reinterpret_cast<VadRecorderWrapperJni *>(handle);
    if (recorderWrapper == nullptr) {
        jniThrowException(env, "java/lang/IllegalStateException", nullptr);
        return JNI_FALSE;
    }

    jboolean ret = JNI_FALSE;
    const char *voiceFileUtf = env->GetStringUTFChars(voiceFilePath,nullptr);
    const char *timestampFileUtf = env->GetStringUTFChars(timestampFilePath,nullptr);
    recorderWrapper->mRecordVoiceFile = fopen(voiceFileUtf, "wb");
    if (recorderWrapper->mRecordVoiceFile == nullptr) {
        pr_err("Failed to open record file: %s", voiceFileUtf);
        goto __error_init;
    }
    recorderWrapper->mRecordTimestampFile = fopen(timestampFileUtf, "w");
    if (recorderWrapper->mRecordTimestampFile == nullptr) {
        pr_err("Failed to open timestamp file: %s", timestampFileUtf);
    }
    recorderWrapper->mVadRecorderListener->setVoiceFile(recorderWrapper->mRecordVoiceFile);
    recorderWrapper->mVadRecorderListener->setTimestampFile(recorderWrapper->mRecordTimestampFile);

    ret = recorderWrapper->mVadRecorder->init(recorderWrapper->mVadRecorderListener,
                                              sampleRate, channels, bitsPerSample);
    if (!ret)
        pr_err("Failed to Init VadRecorder");

__error_init:
    env->ReleaseStringUTFChars(voiceFilePath, voiceFileUtf);
    env->ReleaseStringUTFChars(timestampFilePath, timestampFileUtf);
    return ret;
}

static jboolean VadRecorder_Feed(JNIEnv *env, jobject thiz, jlong handle, jbyteArray buff, jint size)
{
    //pr_dbg("VadRecorderFeed");
    auto recorderWrapper = reinterpret_cast<VadRecorderWrapperJni *>(handle);
    if (recorderWrapper == nullptr) {
        jniThrowException(env, "java/lang/IllegalStateException", nullptr);
        return JNI_FALSE;
    }
    jbyte *pcmBuffer = env->GetByteArrayElements(buff, nullptr);
    jboolean ret = recorderWrapper->mVadRecorder->feed((char *)pcmBuffer, size);
    if (!ret)
        pr_err("Failed to feed pcm data to VadRecorder");
    env->ReleaseByteArrayElements(buff, pcmBuffer, 0);
    return ret;
}

static void VadRecorder_Deinit(JNIEnv *env, jobject thiz, jlong handle)
{
    pr_dbg("VadRecorderDeinit");
    auto recorderWrapper = reinterpret_cast<VadRecorderWrapperJni *>(handle);
    if (recorderWrapper == nullptr) {
        jniThrowException(env, "java/lang/IllegalStateException", nullptr);
        return;
    }
    recorderWrapper->mVadRecorder->deinit();
    if (recorderWrapper->mRecordVoiceFile != nullptr) {
        fclose(recorderWrapper->mRecordVoiceFile);
        recorderWrapper->mRecordVoiceFile = nullptr;
    }
    if (recorderWrapper->mRecordTimestampFile != nullptr) {
        fclose(recorderWrapper->mRecordTimestampFile);
        recorderWrapper->mRecordTimestampFile = nullptr;
    }
}

static void VadRecorder_Destroy(JNIEnv *env, jobject thiz, jlong handle)
{
    pr_dbg("VadRecorderDestroy");
    auto recorderWrapper = reinterpret_cast<VadRecorderWrapperJni *>(handle);
    if (recorderWrapper == nullptr) {
        jniThrowException(env, "java/lang/IllegalStateException", nullptr);
        return;
    }
    delete recorderWrapper;
}

static JNINativeMethod gMethods[] = {
        {"native_create", "()J", (void *)VadRecorder_Create},
        {"native_init", "(JLjava/lang/String;Ljava/lang/String;III)Z", (void *)VadRecorder_Init},
        {"native_feed", "(J[BI)Z", (void *)VadRecorder_Feed},
        {"native_deinit", "(J)V", (void *) VadRecorder_Deinit},
        {"native_destroy", "(J)V", (void *) VadRecorder_Destroy},
};

static int registerNativeMethods(JNIEnv *env, const char *className,JNINativeMethod *getMethods, int methodsNum)
{
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == nullptr) {
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz,getMethods,methodsNum) < 0) {
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv* env = nullptr;
    jint result = -1;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        pr_err("Failed to get env");
        goto bail;
    }
    assert(env != nullptr);
    if (registerNativeMethods(env, JAVA_CLASS_NAME, gMethods, NELEM(gMethods)) != JNI_TRUE) {
        pr_err("Failed to register native methods");
        goto bail;
    }
    /* success -- return valid version number */
    result = JNI_VERSION_1_6;
bail:
    return result;
}
