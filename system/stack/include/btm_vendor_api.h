/******************************************************************************
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 ******************************************************************************/

#pragma once

#include "stack/include/btm_vendor_types.h"

/*******************************************************************************
 *
 * Function         BTM_GetRemoteQLLFeatures
 *
 * Description      This function is called to get remote QLL features
 *
 * Returns          true if feature value is available
 *
 *
 ******************************************************************************/
extern bool BTM_GetRemoteQLLFeatures(uint16_t handle, uint8_t* features);

/*******************************************************************************
 *
 * Function        BTM_IsQHSPhySupported
 *
 * Description     This function is called to determine if QHS is supported or
 *not.
 *
 * Parameters      bda : BD address of the remote device
 *                 transport : Physical transport used for ACL connection
 *                 (BR/EDR or LE)
 *
 * Returns         True if qhs phy can be used, false otherwise.
 *
 ******************************************************************************/
bool BTM_IsQHSPhySupported(const RawAddress& bda, tBT_TRANSPORT transport);

/*******************************************************************************
 *
 * Function         BTM_GetHostAddOnFeatures
 *
 * Description      BTM_GetHostAddOnFeatures
 *
 *
 * Returns          host add on features array
 *
 ******************************************************************************/
bt_device_host_add_on_features_t* BTM_GetHostAddOnFeatures(
    uint8_t* host_add_on_features_len);

/*******************************************************************************
 *
 * Function         BTM_GetSocAddOnFeatures
 *
 * Description      BTM_GetSocAddOnFeatures
 *
 *
 * Returns          soc add on features array
 *
 ******************************************************************************/
bt_device_soc_add_on_features_t* BTM_GetSocAddOnFeatures(
    uint8_t* soc_add_on_features_len);


/*******************************************************************************
 *
 * Function         BTM_GetQllLocalSupportedFeatures
 *
 * Description      BTM_GetQllLocalSupportedFeatures
 *
 *
 * Returns          get QLL Local Supported add on features array
 *
 ******************************************************************************/
bt_device_qll_local_supported_features_t* BTM_GetQllLocalSupportedFeatures(
    uint8_t* qll_local_supported_features_len);


/*******************************************************************************
 *
 * Function         BTM_ReadVendorAddOnFeatures
 *
 * Description      BTM_ReadVendorAddOnFeatures
 *
 * Parameters:      None
 *
 ******************************************************************************/
void BTM_ReadVendorAddOnFeatures();

char* BTM_GetA2dpOffloadCapablity();

bool BTM_IsSpiltA2dpSupported();

bool BTM_IsAACFrameCtrlEnabled();

uint8_t* BTM_GetScramblingSupportedFreqs(uint8_t* number_of_freqs);

/*******************************************************************************
 *
 * Function         BTM_SetPowerBackOffState
 *
 * Description      This function sets PowerBackOff state.
 *
 * Returns          void
 *
 ******************************************************************************/
extern void BTM_SetPowerBackOffState(bool status);
