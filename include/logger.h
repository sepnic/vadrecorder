/*
 * Copyright (C) 2018-2023, Qinglong<sysu.zqlong@gmail.com>
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

#ifndef __VADRECORDER_LOGGER_H
#define __VADRECORDER_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ANDROID)
#include <android/log.h>
#define pr_dbg(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, TAG, fmt, ##__VA_ARGS__)
#define pr_wrn(fmt, ...) __android_log_print(ANDROID_LOG_WARN,  TAG, fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##__VA_ARGS__)

#else
#define pr_dbg(fmt, ...) fprintf(stdout, TAG ": " fmt "\n", ##__VA_ARGS__)
#define pr_wrn(fmt, ...) fprintf(stdout, TAG ": " fmt "\n", ##__VA_ARGS__)
#define pr_err(fmt, ...) fprintf(stderr, TAG ": " fmt "\n", ##__VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif

#endif // __VADRECORDER_LOGGER_H
