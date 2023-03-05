/*
 * Copyright (C) 2023-, Qinglong<sysu.zqlong@gmail.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "lockfree_ringbuf.h"

struct lockfree_ringbuf {
    char *p_o;                   /**< Original pointer */
    char *volatile p_r;          /**< Read pointer */
    char *volatile p_w;          /**< Write pointer */
    // TODO: make fill_cnt atomic, since both read thread and write thread will modify it
    int  fill_cnt;               /**< Number of filled slots */
    int  size;                   /**< Buffer size */
    bool allow_overwrite;
};

void *lockfree_ringbuf_create(int size)
{
    if (size <= 0)
        return NULL;
    struct lockfree_ringbuf *rb = malloc(sizeof(struct lockfree_ringbuf));
    if (rb == NULL)
        return NULL;
    rb->size = size;
    rb->fill_cnt = 0;
    rb->allow_overwrite = false;
    rb->p_o = rb->p_r = rb->p_w = malloc(size);
    if (rb->p_o == NULL) {
        free(rb);
        rb = NULL;
    }
    return rb;
}

void lockfree_ringbuf_destroy(void *handle)
{
    struct lockfree_ringbuf *rb = (struct lockfree_ringbuf *)handle;
    if (rb == NULL)
        return;
    if (rb->p_o != NULL)
        free(rb->p_o);
    free(rb);
}

int lockfree_ringbuf_get_size(void *handle)
{
    struct lockfree_ringbuf *rb = (struct lockfree_ringbuf *)handle;
    if (rb == NULL)
        return LOCKFREE_RINGBUF_ERROR_INVALID_PARAMETER;
    return rb->size;
}

int lockfree_ringbuf_bytes_available(void *handle)
{
    struct lockfree_ringbuf *rb = (struct lockfree_ringbuf *)handle;
    if (rb == NULL)
        return LOCKFREE_RINGBUF_ERROR_INVALID_PARAMETER;
    return (rb->size - rb->fill_cnt);
}

int lockfree_ringbuf_bytes_filled(void *handle)
{
    struct lockfree_ringbuf *rb = (struct lockfree_ringbuf *)handle;
    if (rb == NULL)
        return LOCKFREE_RINGBUF_ERROR_INVALID_PARAMETER;
    return rb->fill_cnt;
}

void lockfree_ringbuf_allow_unsafe_overwrite(void *handle, bool allow)
{
    struct lockfree_ringbuf *rb = (struct lockfree_ringbuf *)handle;
    if (rb == NULL)
        return;
    rb->allow_overwrite = allow;
}

void lockfree_ringbuf_unsafe_reset(void *handle)
{
    struct lockfree_ringbuf *rb = (struct lockfree_ringbuf *)handle;
    if (rb == NULL)
        return;
    rb->p_r = rb->p_w = rb->p_o;
    rb->fill_cnt = 0;
}

int lockfree_ringbuf_unsafe_discard(void *handle, int len)
{
    struct lockfree_ringbuf *rb = (struct lockfree_ringbuf *)handle;
    if (rb == NULL || len <= 0)
        return LOCKFREE_RINGBUF_ERROR_INVALID_PARAMETER;
    len = (len > rb->fill_cnt) ? rb->fill_cnt : len;
    if (len > 0) {
        if ((rb->p_r + len) > (rb->p_o + rb->size)) {
            int rlen1 = rb->p_o + rb->size - rb->p_r;
            int rlen2 = len - rlen1;
            rb->p_r = rb->p_o + rlen2;
        } else {
            rb->p_r = rb->p_r + len;
        }
        rb->fill_cnt -= len;
    }
    return len;
}

int lockfree_ringbuf_read(void *handle, char *buf, int len)
{
    struct lockfree_ringbuf *rb = (struct lockfree_ringbuf *)handle;
    if (rb == NULL || buf == NULL || len <= 0)
        return LOCKFREE_RINGBUF_ERROR_INVALID_PARAMETER;
    len = (len > rb->fill_cnt) ? rb->fill_cnt : len;
    if (len > 0) {
        if ((rb->p_r + len) > (rb->p_o + rb->size)) {
            int rlen1 = rb->p_o + rb->size - rb->p_r;
            int rlen2 = len - rlen1;
            memcpy(buf, rb->p_r, rlen1);
            memcpy(buf + rlen1, rb->p_o, rlen2);
            rb->p_r = rb->p_o + rlen2;
        } else {
            memcpy(buf, rb->p_r, len);
            rb->p_r = rb->p_r + len;
        }
        rb->fill_cnt -= len;
    }
    return len;
}

int lockfree_ringbuf_write(void *handle, char *buf, int len)
{
    struct lockfree_ringbuf *rb = (struct lockfree_ringbuf *)handle;
    if (rb == NULL || buf == NULL || len <= 0)
        return LOCKFREE_RINGBUF_ERROR_INVALID_PARAMETER;
    if (len < rb->size) {
        int available = lockfree_ringbuf_bytes_available(rb);
        if (available < len) {
            if (!rb->allow_overwrite)
                return LOCKFREE_RINGBUF_ERROR_WRITTEN_SIZE_EXCEED_BUFFER_AVAILABLE;
            lockfree_ringbuf_unsafe_discard(rb, len-available);
        }
        if ((rb->p_w + len) > (rb->p_o + rb->size)) {
            int wlen1 = rb->p_o + rb->size - rb->p_w;
            int wlen2 = len - wlen1;
            memcpy(rb->p_w, buf, wlen1);
            memcpy(rb->p_o, buf + wlen1, wlen2);
            rb->p_w = rb->p_o + wlen2;
        } else {
            memcpy(rb->p_w, buf, len);
            rb->p_w = rb->p_w + len;
        }
        rb->fill_cnt += len;
    } else {
        if (!rb->allow_overwrite)
            return LOCKFREE_RINGBUF_ERROR_WRITTEN_SIZE_EXCEED_BUFFER_SIZE;
        buf = buf + len - rb->size;
        len = rb->size;
        memcpy(rb->p_o, buf, len);
        rb->p_w = rb->p_o + len;
        rb->p_r = rb->p_o;
        rb->fill_cnt = len;
    }
    return len;
}
