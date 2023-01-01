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

#include <poll.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "IqiBpRedirector.h"
#include "StreamBridgeModem.h"
#include "StreamBridgeSerial.h"

static StreamBridge *g_pModemBridge = 0;
static StreamBridge *g_pSerialBridge = 0;

const static char* TAG = "IqiBpRedirector";

IqiBpRedirector::IqiBpRedirector(): mReady(false) {
    IQI_LOGD(TAG, "Start to initialize IqiBpRedirector");

    memset(&mModemBridgeThread, 0, sizeof(pthread_t));
    memset(&mSerialBridgeThread, 0, sizeof(pthread_t));

    if (!initSerialBridge()) {
        IQI_LOGE(TAG, "initSerialBridge() failed!");
        return;
    }

    if (!initModemBridge()) {
        IQI_LOGE(TAG, "initModemBridge() failed!");
        return;
    }
    mReady = true;
}

IqiBpRedirector::~IqiBpRedirector() {
    if (g_pModemBridge != NULL) {
        g_pModemBridge->closeStream();
        delete g_pModemBridge;
    }
    if (g_pSerialBridge != NULL) {
        g_pSerialBridge->closeStream();
        delete g_pSerialBridge;
    }

    mReady = false;
}

bool IqiBpRedirector::isReady() {
    return mReady;
}

bool IqiBpRedirector::initModemBridge() {
    if (g_pModemBridge != NULL) {
        IQI_LOGD(TAG, "[MODEM] bridge has been initialized");
        return true;
    }
    g_pModemBridge = new StreamBridgeModem();
    if (!g_pModemBridge->openStream()) {
        delete g_pModemBridge;
        return false;
    }

    int *bridgeType = (int *)calloc(0, sizeof(int));
    if (bridgeType == NULL) {
        IQI_LOGD(TAG, "[MODEM] OOM!");
        return false;
    }

    *bridgeType = STREAM_BRIDGE_TYPE_MODEM;
    pthread_create(&mModemBridgeThread, NULL, bridgeThreadFunc, (void *)bridgeType);
    pthread_setname_np(mModemBridgeThread, THREAD_NAME_MODEM_BRIDGE);
    IQI_LOGD(TAG, "[MODEM] bridge initialized successfully");

    return true;
}

bool IqiBpRedirector::initSerialBridge() {
    // Initialize serial port so that CADeT can access it.
    if (g_pSerialBridge != NULL) {
        IQI_LOGD(TAG, "[SERIAL] serial bridge has been initialized");
        return false;
    }
    g_pSerialBridge = new StreamBridgeSerial();
    if (!g_pSerialBridge->openStream()) {
        delete g_pSerialBridge;
        return false;
    }

    int *bridgeType = (int *)calloc(0, sizeof(int));
    if (bridgeType == NULL) {
        IQI_LOGD(TAG, "[SERIAL] OOM!");
        return false;
    }

    *bridgeType = STREAM_BRIDGE_TYPE_SERIAL;
    pthread_create(&mSerialBridgeThread, NULL, bridgeThreadFunc, (void *)bridgeType);
    pthread_setname_np(mSerialBridgeThread, THREAD_NAME_SERIAL_BRIDGE);
    IQI_LOGD(TAG, "[SERIAL] bridge initialized successfully");
    return true;
}

void *IqiBpRedirector::bridgeThreadFunc(void *param) {
    int *pBridgeType = NULL;
    int bridgeType = STREAM_BRIDGE_TYPE_NONE;

    if (param != NULL) {
        pBridgeType = (int *)param;
        bridgeType = (int)*pBridgeType;
        free(param);
    } else {
        IQI_LOGE(TAG, "bridgeThreadFunc() exit with NULL param!");
        exit(EXIT_FAILURE);
    }

    struct pollfd fds[1];
    int poll_ret = -1;
    int streamFD = -1;
    short streamEvents = POLLIN;
    int timeout = -1;
    char TYPE_TAG[32];
    unsigned char data[STREAM_BRIDGE_BUFFER_SIZE];

    if (bridgeType == STREAM_BRIDGE_TYPE_MODEM) {
        streamFD = g_pModemBridge->getStreamFd();
        sprintf((char *)TYPE_TAG, LOG_TAG_MODEM_BRIDGE);
    } else if (bridgeType == STREAM_BRIDGE_TYPE_SERIAL) {
        streamFD = g_pSerialBridge->getStreamFd();
        sprintf((char *)TYPE_TAG, LOG_TAG_SERIAL_BRIDGE);
    } else {
        IQI_LOGE(TAG, "[%s] bridgeThreadFunc() failed with invalid bridge type = %d", TYPE_TAG, bridgeType);
        return NULL;
    }

    IQI_LOGD(TAG, "[%s] bridgeThreadFunc() enter loop", TYPE_TAG);

    while (true) {
        fds[0].fd = streamFD;
        fds[0].events = streamEvents;

        IQI_LOGD(TAG, "[%s] begin pull with fd = %d", TYPE_TAG, streamFD);
        poll_ret = poll(fds, 1, timeout);
        IQI_LOGD(TAG, "[%s] end pull", TYPE_TAG);

        if (poll_ret < 0) {
            IQI_LOGE(TAG, "[%s] poll error, %s, errno = %d", TYPE_TAG, strerror(errno), errno);
        } else {
            if (fds[0].revents & POLLIN) {
                // Clear data fisrt
                memset(data, 0, STREAM_BRIDGE_BUFFER_SIZE);

                // Read from source
                unsigned long bytes_read = STREAM_BRIDGE_BUFFER_SIZE;

                if (bridgeType == STREAM_BRIDGE_TYPE_MODEM) {
                    g_pModemBridge->readStream(data, &bytes_read);
                    IQI_LOGD(TAG, "[BP->REDIR] read: %lu", bytes_read);
                } else if (bridgeType == STREAM_BRIDGE_TYPE_SERIAL) {
                    g_pSerialBridge->readStream(data, &bytes_read);
                    IQI_LOGD(TAG, "[CADET->REDIR] read: %lu", bytes_read);
                }
                if (DUMP_BRIDGE_DATA) {
                    printBuf((const unsigned char *)TYPE_TAG, data, bytes_read);
                }

                // Redirect to target
                if (bytes_read < 0) {
                    IQI_LOGE(TAG, "[%s] read error, %s, errno = %d", TYPE_TAG, strerror(errno), errno);
                } else {
                    unsigned long bytes_written = bytes_read;

                    if (bridgeType == STREAM_BRIDGE_TYPE_MODEM) {
                        g_pSerialBridge->writeStream(data, &bytes_written);
                        IQI_LOGD(TAG, "[REDIR->CADeT] write: %lu", bytes_written);
                    } else if (bridgeType == STREAM_BRIDGE_TYPE_SERIAL) {
                        g_pModemBridge->writeStream(data, &bytes_written);
                        IQI_LOGD(TAG, "[REDIR->BP] write: %lu", bytes_written);
                    }
                    if (bytes_read > bytes_written) {
                        unsigned long bytes_missed = bytes_read - bytes_written;
                        IQI_LOGW(TAG, "[%s] missed: %lu", TYPE_TAG, bytes_missed);
                    }
                }
            }
        }
    }
    return NULL;
}

void IqiBpRedirector::printBuf(const unsigned char *prefix, unsigned char *buf, unsigned int buf_len) {
    char *result = NULL;

    if (printBufHex(buf, buf_len, &result)) {
        IQI_LOGD(TAG, "[%s] 0x%s", prefix, result);
        free(result);
    }
}

bool IqiBpRedirector::printBufHex(const unsigned char *bin, unsigned int binsz, char **result) {
    char hex_str[]= "0123456789ABCDEF";
    unsigned int  i;

    if (binsz == 0) {
        IQI_LOGE(TAG, "Zero binsz");
        return false;
    }

    *result = (char *)calloc(binsz * 2 + 1, sizeof(char));
    if (*result == NULL) {
        IQI_LOGE(TAG, "printBufHex() OOM!");
        return false;
    }

    (*result)[binsz * 2] = 0;

    for (i = 0; i < binsz; i++) {
        (*result)[i * 2 + 0] = hex_str[(bin[i] >> 4) & 0x0F];
        (*result)[i * 2 + 1] = hex_str[(bin[i]     ) & 0x0F];
    }

    return true;
}