/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2019. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include "StreamBridge.h"

const static char* TAG = "StreamBridge";

StreamBridge::StreamBridge():
        maxWritableLen(MAX_BRIDGE_STREAM_RW_SIZE), mStreamFd(INVALID_STREAM_FD) {
}

StreamBridge::~StreamBridge() {
    if (mStreamFd >= 0) {
        close(mStreamFd);
        mStreamFd = INVALID_STREAM_FD;
    }
    close(monitorSocketFd);
    close(agentSocketFd);
}

bool StreamBridge::openStream() {
    int aErrno;
    mStreamFd = openInternal(&aErrno);

    if (mStreamFd < 0) {
        if (aErrno == EAGAIN || aErrno == EWOULDBLOCK) {
            IQI_LOGW(TAG, "openStream() async pending");
            return false;
        }
        IQI_LOGE(TAG, "openStream() failed");
        return false;
    }

    IQI_LOGD(TAG, "openStream(%d) successfully", mStreamFd);
    return true;
}

bool StreamBridge::closeStream() {
    if (mStreamFd < 0) {
        IQI_LOGD(TAG, "Invalid stream FD while closing");
        return false;
    }

    if (close(mStreamFd)) {
        IQI_LOGD(TAG, "cannot close(%d),  %s, errno = %d", mStreamFd, strerror(errno), errno);
        mStreamFd = INVALID_STREAM_FD;
        return false;
    }

    IQI_LOGD(TAG, "closeStream(%d) successfully", mStreamFd);
    return true;
}

bool StreamBridge::readStream(void *ioBuffer, unsigned long *ioLen) {
    ssize_t bytes_read = read(mStreamFd, ioBuffer, *ioLen);

    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            IQI_LOGW(TAG, "readStream() async pending");
            return false;
        }
        IQI_LOGE(TAG, "read error, %s, errno = %d", strerror(errno), errno);
        return false;
    }

    if (bytes_read == 0) {
        *ioLen = 0;
        return false;
    }

    *ioLen = bytes_read;
    return true;
}

bool StreamBridge::writeStream(const void *ioBuffer, unsigned long *ioLen) {
    if (*ioLen > maxWritableLen) {
        *ioLen = maxWritableLen;
    }

    ssize_t bytes_written = write(mStreamFd, ioBuffer, *ioLen);

    if (bytes_written < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            IQI_LOGW(TAG, "writeStream() async pending");
            return false;
        }
        IQI_LOGE(TAG, "write error, %s, errno = %d", strerror(errno), errno);
        return false;
    }

    if (bytes_written == 0) {
        *ioLen = 0;
        return false;
    }

    *ioLen = bytes_written;
    return true;
}

void StreamBridge::sendToMonitor(char code) {
    IQI_LOGD(TAG, "sendToMonitor --> %d", code);
    write(agentSocketFd, &code, 1);
}
