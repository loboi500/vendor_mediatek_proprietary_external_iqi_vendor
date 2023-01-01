
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

#ifndef __IQI_BP_REDIRECTOR_H__
#define __IQI_BP_REDIRECTOR_H__

#include "IqiUtils.h"
#include "StreamBridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define DUMP_BRIDGE_DATA (1)
#define STREAM_BRIDGE_TYPE_NONE (-1)
#define STREAM_BRIDGE_TYPE_MODEM (0)
#define STREAM_BRIDGE_TYPE_SERIAL (1)
#define STREAM_BRIDGE_BUFFER_SIZE (512)
#define THREAD_NAME_MODEM_BRIDGE "modem-bridge"
#define THREAD_NAME_SERIAL_BRIDGE "serial-bridge"
#define LOG_TAG_MODEM_BRIDGE "MODEM"
#define LOG_TAG_SERIAL_BRIDGE "SERIAL"


/**
 * This module is designed for iQI BP test,
 * forwarding data between CADeT tool and Baseband Platform (BP).
 * CADeT <--Serial Bridge--> iQiBpRedirector <--Modem Bridge--> BP
 */

class IqiBpRedirector {
public:
    IqiBpRedirector();
    ~IqiBpRedirector();

    bool isReady();

private:
    bool mReady;
    pthread_t mModemBridgeThread;
    pthread_t mSerialBridgeThread;

    /**
     * Initialize modem bridge and
     * forwarding data between CADeT tool and Baseband Platform (BP).
     * We plan to add new CCCI device for BP in the future.
     */
    bool initModemBridge();

    /**
     * Initialize modem bridge and
     * forwarding data between CADeT tool and Baseband Platform (BP).
     * We plan to add new USB port for BP in the future.
     */
    bool initSerialBridge();

    // Working thread to read/write data
    static void *bridgeThreadFunc(void *param);

    static void printBuf(const unsigned char *prefix, unsigned char *buf, unsigned int buf_len);
    static bool printBufHex(const unsigned char *bin, unsigned int binsz, char **result);
};

#endif // __IQI_BP_REDIRECTOR_H__
