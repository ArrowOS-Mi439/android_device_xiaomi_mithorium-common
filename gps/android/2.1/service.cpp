/*

 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Not a Contribution
 */
/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2_0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2_0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.gnss@2.1-service-qti"

#include <android/hardware/gnss/2.1/IGnss.h>
#include <hidl/LegacySupport.h>
#include "loc_cfg.h"
#include "loc_misc_utils.h"
#include <dlfcn.h>


extern "C" {
#include "vndfwk-detect.h"
}

#ifdef ARCH_ARM_32
#define DEFAULT_HW_BINDER_MEM_SIZE 65536
#endif

using android::hardware::gnss::V2_1::IGnss;

using android::hardware::configureRpcThreadpool;
using android::hardware::registerPassthroughServiceImplementation;
using android::hardware::joinRpcThreadpool;

using android::status_t;
using android::OK;

typedef int vendorEnhancedServiceMain(int /* argc */, char* /* argv */ []);

#define GNSS_AUTO_POWER_LIBNAME  "libgnssauto_power.so"
#define GNSS_WEAR_POWER_LIBNAME  "libgnsswear_power.so"

typedef const void* (*gnssPowerHandler)(void);

int initializeGnssAutoPowerHandler() {

    void * handle = nullptr;
    gnssPowerHandler getter = (gnssPowerHandler) dlGetSymFromLib(handle, GNSS_AUTO_POWER_LIBNAME,
                                                                 "initGnssAutoPowerHandler");
    if (nullptr != getter) {
        getter();
        ALOGI("GnssAutoPowerHandler Initialized!");
        return 0;
    }
    return -1;
}

int initializeGnssWearPowerHandler() {

    void * handle = nullptr;
    gnssPowerHandler getter = (gnssPowerHandler) dlGetSymFromLib(handle, GNSS_WEAR_POWER_LIBNAME,
                                                                 "initGnssWearPowerHandler");
    if (nullptr != getter) {
        getter();
        ALOGI("GnssWearPowerHandler Initialized!");
        return 0;
    }
    return -1;
}

void initializeGnssPowerHandler() {

    if (0 != initializeGnssAutoPowerHandler()) {
        ALOGW("Gnss Auto Power Handler unavailable.");

        if (0 != initializeGnssWearPowerHandler()) {
            ALOGW("Gnss Wear Power Handler unavailable.");
        }
    }
}

int main() {

    ALOGI("%s", __FUNCTION__);

    int vendorInfo = getVendorEnhancedInfo();
    bool vendorEnhanced = ( 1 == vendorInfo || 3 == vendorInfo );
    setVendorEnhanced(vendorEnhanced);

#ifdef ARCH_ARM_32
    android::hardware::ProcessState::initWithMmapSize((size_t)(DEFAULT_HW_BINDER_MEM_SIZE));
#endif
    configureRpcThreadpool(1, true);
    status_t status;

    status = registerPassthroughServiceImplementation<IGnss>();
    if (status == OK) {
    #ifdef LOC_HIDL_VERSION
        #define VENDOR_ENHANCED_LIB "vendor.qti.gnss@" LOC_HIDL_VERSION "-service.so"

        void* libHandle = NULL;
        vendorEnhancedServiceMain* vendorEnhancedMainMethod = (vendorEnhancedServiceMain*)
                dlGetSymFromLib(libHandle, VENDOR_ENHANCED_LIB, "main");
        if (NULL != vendorEnhancedMainMethod) {
            (*vendorEnhancedMainMethod)(0, NULL);
        }
    #else
        ALOGI("LOC_HIDL_VERSION not defined.");
    #endif
        initializeGnssPowerHandler();
        joinRpcThreadpool();
    } else {
        ALOGE("Error while registering IGnss 2.1 service: %d", status);
    }

    return 0;
}
