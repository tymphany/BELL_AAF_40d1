/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      GAA internal API
*/

#include "ui.h"
#include "gaa_debug.h"
#include "voice_ui_container.h"

#ifndef GAA_PRIVATE_H_
#define GAA_PRIVATE_H_

/*! \brief Internal message for the GAA module. */
enum gaa_internal_messages
{
    GAA_INTERNAL_START_QUERY_IND,
    GAA_INTERNAL_END_QUERY_IND,
    GAA_INTERNAL_START_RESPONSE_IND,
    GAA_INTERNAL_END_RESPONSE_IND,
    GAA_INTERNAL_TWS_DISCONNECTION_IND,
    GAA_INTERNAL_TWS_DISCONNECTION_TIMEOUT,
#ifdef ENABLE_TYM_PLATFORM    /*added Qualcomm patch,for stop bisto*/
    GAA_INTERNAL_STOP_ASSISTANT
#endif    
};

/*! \brief Initialise the service to enabled/disabled dependent on configuration
*/
void Gaa_InitialiseService(bool gaa_enabled);

/*! \brief Notify the GAA library that a link has disconnected for a given BT address
    \param bd_addr BT address of the other device
*/
void Gaa_OnLinkDisconnected(bdaddr *bd_addr);

/*! \brief Notify the GAA library that a link has connected for a given BT address
    \param bd_addr BT address of the other device
*/
void Gaa_OnLinkConnected(bdaddr *bd_addr);

/*! \brief Initialise the BT RFCOMM channels
*/
void Gaa_InitialiseChannels(void);

/*! \brief Get the GAA action mapping stored in non-volatile memory
    \return uint8 Index to select either the one, three or five button default action mapping
*/
uint8 Gaa_GetActionMapping(void);

/*! \brief Store the GAA state in non-volatile memory
    \param State to set the GAA enable
*/
void Gaa_SetEnabledState(bool enabled_state);

/*! \brief Get the device Model ID from config
    \param[out] uint32 pointer to be populated with model id
    \return bool TRUE if valid model id was found, FALSE if not
*/
bool Gaa_GetModelId(uint32 *model_id);

/*! \brief Pass the VA user event to GAA
    \param[in] ui_input_t user input event
    \return bool TRUE if event was handled
*/
bool Gaa_HandleVaEvent(ui_input_t voice_assistant_user_event);

/*! \brief Close the GAA audio in
*/
void Gaa_AudioInClose(void);

/*! \brief Get the audio task for internal messaging
    \return TaskData * audio task
*/
TaskData *Gaa_GetAudioTask(void);

/*! \brief Check if an reboot was due to an OTA upgrade
    \return bool TRUE if reboot was due to OTA upgrade
*/
bool GaaRebootDueToOtaUpgrade(void);

/*! \brief Notify GSound transmission over BLE completed
    \param[in] uint16 BLE channel type (control/audio)
    \param[in] uint8 BLE data
    \param[in] uint16 length of BLE data
*/
void Gaa_BleTxDone(uint16 channel_type, const uint8 *data, uint16 length);

/*! \brief Notify GSound data received from remote source
    \param[in] uint16 BLE channel type (control/audio)
    \param[in] uint8 BLE data
    \param[in] uint16 length of BLE data
*/
void Gaa_BleOnRxReady(uint16 channel_type, const uint8 *data, uint16 length);

/*! \brief Notify GSound BLE channel has disconnected
    \param[in] uint16 BLE channel type (control/audio)
*/
void Gaa_BleDisconnectCfm(uint16 channel_type);

/*! \brief Notify GSound BLE channel is connecting
    \param[in] uint16 BLE channel type (control/audio)
    \param[in] bdaddr Bluetooth address
    \return bool TRUE if connection was accepted
*/
bool Gaa_BleConnectInd(uint16 channel_type, bdaddr *bd_addr);

/*! \brief Notify GSound that pace is available in the connected sink for transmission
*/
void Gaa_BleTxAvailable(void);

/*! \brief Register for LE Advertising Manager
*/
void Gaa_BleRegisterAdvertising(void);

/*! \brief Get the tws task for internal messaging
    \return TaskData * tws task
*/
TaskData *Gaa_GetTwsTask(void);

/*! \brief Gets the voiceui protected interface for GAA component
    \return Pointer to voice_ui_protected_if_t
*/
voice_ui_protected_if_t *Gaa_GetVoiceUiProtectedInterface(void);

#endif  /* GAA_PRIVATE_H_ */
