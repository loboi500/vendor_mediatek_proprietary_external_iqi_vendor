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

#include "StreamBridgeSerial.h"

#define TAG "StreamBridgeSerial"

StreamBridgeSerial::StreamBridgeSerial() {}

bool StreamBridgeSerial::closeStream() {
    return StreamBridge::closeStream();
}

int StreamBridgeSerial::openInternal(int* aErrno) {
    // From Android Q, vendor module can't set this system property.
    // Go to EM APP > Others > USB ACM to open serial port before lunch.
    //rfx_property_set("sys.usb.config", "gs3");

    IQI_LOGD(TAG, "Open device %s", DEVICE_SERIAL_BRIDGE);
    int fd = open(DEVICE_SERIAL_BRIDGE, O_RDWR);

    if (SET_SERIAL_RAW_MODE) {
        IQI_LOGD(TAG, "Set tty device to raw mode");

        struct termios ti;
        memset(&ti, 0, sizeof(ti));

        tcflush(fd, TCIOFLUSH);
        // Get current device settings
        if (tcgetattr(fd, &ti) < 0) {
            IQI_LOGE(TAG, "Failed to open serial port setting!");
            close(fd);
            return INVALID_STREAM_FD;
        }

        // Set raw mode
        cfmakeraw(&ti);

        // Set I/O baudrate
        /*
        if (cfsetospeed(&ti, CADET_BAUDRATE) < 0) {
            IQI_LOGE(TAG, "Failed to set serial output baudrate");
        }
        if (cfsetispeed(&ti, CADET_BAUDRATE) < 0) {
            IQI_LOGE(TAG, "Failed to set serial intput baudrate");
        }
        */
        if (tcsetattr(fd, TCSANOW, &ti) < 0) {
            IQI_LOGE(TAG, "Failed to set serial attr to raw mode");
            close(fd);
            return INVALID_STREAM_FD;
        }

        tcflush(fd, TCIOFLUSH);
    }
    *aErrno = errno;
    return fd;
}
