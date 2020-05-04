/*******************************************************************************
Copyright (c) 2018 - 2020 Qualcomm Technologies International, Ltd.

FILE NAME
    gaa_ble.c

DESCRIPTION
    Implementation for GAA BLE

NOTES
    See gsound_target_ble.h for API documentation
*/

#include <sink.h>
#include <stream.h>
#include <panic.h>
#include <logging.h>
#include <connection_manager.h>
#include "gatt_gaa_comm_server.h"
#include "gsound_target_ble.h"
#include "gaa_private.h"
#include "gaa_config.h"
#include "gaa_connect_state.h"
#include "le_advertising_manager.h"
#include "local_name.h"

#define NUMBER_OF_ADVERT_DATA_ITEMS 1

#define GAA_SERVICE_UUID (0xFE26)
#define GAA_MODEL_ID_OFFSET 4
#define GAA_PAYLOAD_SIZE_OFFSET 0


static const GSoundBleInterface *ble_handler;

/********************************************************************
* Local functions prototypes:
*/
static void gaa_BleConvertBdAddrToGaaType(GSoundBTAddr *gaa_bd_addr, bdaddr *bd_addr);
static unsigned int gaa_BleNumberOfAdvItems(const le_adv_data_params_t * params);
static le_adv_data_item_t gaa_BleGetAdvDataItems(const le_adv_data_params_t * params, unsigned int id);
static void gaa_BleReleaseAdvDataItems(const le_adv_data_params_t * params);


static le_adv_data_callback_t gaa_le_advert_callback =
{
    .GetNumberOfItems = gaa_BleNumberOfAdvItems,
    .GetItem = gaa_BleGetAdvDataItems,
    .ReleaseItems = gaa_BleReleaseAdvDataItems
};

static uint8 gaa_service_adv_data[] = {
    0x00,
    ble_ad_type_service_data,
    (GAA_SERVICE_UUID & 0xFF),
    (GAA_SERVICE_UUID >> 8),
    0x00,
    0x00,
    0x00
};
#define SIZE_GAA_SERVICE_DATA (sizeof(gaa_service_adv_data)/sizeof(gaa_service_adv_data[0]))

static le_adv_data_item_t gaa_adv_data = {0};

/********************************************************************
* Local functions:
*/

/*********************************************************************/

static le_adv_data_item_t gaa_BleGetAdvServiceItem(void)
{
    if(gaa_adv_data.data == NULL)
    {
        uint32 model_id;
        Gaa_GetModelId(&model_id);
        gaa_service_adv_data[GAA_MODEL_ID_OFFSET] = model_id & 0xFF;
        gaa_service_adv_data[GAA_MODEL_ID_OFFSET+ 1] = (model_id >> 8) & 0xFF;
        gaa_service_adv_data[GAA_MODEL_ID_OFFSET+ 2] = (model_id >> 16) & 0xFF;

        gaa_service_adv_data[GAA_PAYLOAD_SIZE_OFFSET] = SIZE_GAA_SERVICE_DATA - 1;

        gaa_adv_data.size = SIZE_GAA_SERVICE_DATA;
        gaa_adv_data.data = gaa_service_adv_data;
    }
    return gaa_adv_data;
}

/*********************************************************************/
static void gaa_BleConvertBdAddrToGaaType(GSoundBTAddr *gaa_bd_addr, bdaddr *bd_addr)
{
    gaa_bd_addr->lap = bd_addr->lap;
    gaa_bd_addr->nap = bd_addr->nap;
    gaa_bd_addr->uap = bd_addr->uap;
}


/*********************************************************************/
static bool gaa_BleIsRequestValidForGaaDataSet(const le_adv_data_params_t * params)
{
    return ((params->data_set == le_adv_data_set_handset_identifiable) || (params->data_set == le_adv_data_set_handset_unidentifiable)) &&
            (params->completeness == le_adv_data_completeness_full) &&
            (params->placement == le_adv_data_placement_dont_care);
}

/*********************************************************************/
static unsigned int gaa_BleNumberOfAdvItems(const le_adv_data_params_t * params)
{
    return gaa_BleIsRequestValidForGaaDataSet(params) ? NUMBER_OF_ADVERT_DATA_ITEMS : 0;
}

/*********************************************************************/
static le_adv_data_item_t gaa_BleGetAdvDataItems(const le_adv_data_params_t * params, unsigned int id)
{
    UNUSED(id);
    le_adv_data_item_t no_data = {.size = 0, .data = NULL};
    return gaa_BleIsRequestValidForGaaDataSet(params) ? gaa_BleGetAdvServiceItem() : no_data;
}

/*********************************************************************/
static void gaa_BleReleaseAdvDataItems(const le_adv_data_params_t * params)
{
    UNUSED(params);

    return;
}

/* Request Quality of Service suitable for audio transmission ********/
static void gaa_BleRequestQosForAudio(void)
{
    tp_bdaddr tpaddr;
    
    if (VmGetBdAddrtFromCid(GattGaaCommServerGetCid(), &tpaddr))
    {
        ConManagerRequestDeviceQos(&tpaddr, cm_qos_audio);
    }
    else
    {
        DEBUG_LOG("gaa_RequestQosForAudio: no address");
    }
}

/* Release Audio QoS requirement *************************************/
static void gaa_BleReleaseQosForAudio(void)
{
    tp_bdaddr tpaddr;
    
    if (VmGetBdAddrtFromCid(GattGaaCommServerGetCid(), &tpaddr))
    {
        ConManagerReleaseDeviceQos(&tpaddr, cm_qos_audio);
    }
    else
    {
        DEBUG_LOG("gaa_ReleaseQosForAudio: no address");
    }
}

/*********************************************************************/
void Gaa_BleRegisterAdvertising(void)
{
    LeAdvertisingManager_Register(NULL, &gaa_le_advert_callback);
}

/*********************************************************************/
void Gaa_BleTxAvailable(void)
{
    ble_handler->gsound_ble_on_tx_available(GSOUND_CHANNEL_CONTROL);
    ble_handler->gsound_ble_on_tx_available(GSOUND_CHANNEL_AUDIO);
}

/*********************************************************************/
bool Gaa_BleConnectInd(uint16 channel_type, bdaddr *bd_addr)
{
    GSoundBTAddr gaa_bd_addr;
    bool status = FALSE;

    gaa_BleConvertBdAddrToGaaType(&gaa_bd_addr,bd_addr);
    status = ble_handler->gsound_ble_on_connect((GSoundChannelType)channel_type, &gaa_bd_addr) == GSOUND_STATUS_OK;

    if (status)
    {
        Gaa_SetChannelConnected(channel_type, TRUE);

        if (channel_type == GSOUND_CHANNEL_AUDIO)
        {
            /* This should be done in GSoundTargetAudioPrepareForStreaming() but successive
             * connection updates can result in link loss with some devices */
            gaa_BleRequestQosForAudio();
        }
    }

    DEBUG_LOG("Gaa_BleConnectInd(%d, %04x %02x %06x) = %d",
           channel_type,
           bd_addr->nap,
           bd_addr->uap,
           bd_addr->lap,
           status);

    return status;
}

/*********************************************************************/
void Gaa_BleDisconnectCfm(uint16 channel_type)
{
    DEBUG_LOG("Gaa_BleDisconnectCfm ch=%u", channel_type);
    ble_handler->gsound_ble_on_disconnect((GSoundChannelType)channel_type);
    Gaa_SetChannelConnected(channel_type, FALSE);

    if (channel_type == GSOUND_CHANNEL_AUDIO)
    {
        /* This should be done in GSoundTargetAudioInClose() but successive
         * connection updates can result in link loss with some devices */
        gaa_BleReleaseQosForAudio();
    }
}

/*********************************************************************/
void Gaa_BleOnRxReady(uint16 channel_type, const uint8 *data, uint16 length)
{
   DEBUG_LOG("Gaa_BleOnRxReady ch=%u len=%u", channel_type, length);
    ble_handler->gsound_ble_on_rx_ready((GSoundChannelType)channel_type, data, length);
}

/*********************************************************************/
void Gaa_BleTxDone(uint16 channel_type, const uint8 *data, uint16 length)
{
    DEBUG_LOG("Gaa_BleTxDone");
    ble_handler->gsound_ble_on_tx_consumed((GSoundChannelType)channel_type, data, length);
}

/********************************************************************
* Public function implementations: See gsound_target_ble.h
*/

GSoundStatus GSoundTargetBleInit(const GSoundBleInterface *handlers)
{
    DEBUG_LOG("Initialising Gaa BLE");
    ble_handler = handlers;
    return GSOUND_STATUS_OK;
}

/*********************************************************************/
GSoundStatus GSoundTargetBleTransmit(GSoundChannelType channel,
                                     const uint8_t *data,
                                     uint32_t length,
                                     uint32_t *bytes_consumed)
{
    gatt_gaa_tx_status_t tx_status = GaaCommServerSendNotification(channel, data, length);
    GSoundStatus status = GSOUND_STATUS_ERROR;

    DEBUG_LOG("GSoundTargetBleTransmit; ch=%u len=%u sts=%u", channel, length, tx_status);

    *bytes_consumed = 0;

    switch (tx_status)
    {
    case tx_failed_no_space_available:
        status = GSOUND_STATUS_OUT_OF_MEMORY;
        break;

    case tx_success:
        status = GSOUND_STATUS_OK;
        *bytes_consumed = length;
        break;

    case tx_failed_sink_invalid:
        /* Gaa on receiving a GSOUND_STATUS_ERROR will attempt another transmission,
         * thus a call to disconnect is required to stop any subsequent calls.
         */
        ble_handler->gsound_ble_on_disconnect(channel);
        break;
    default:
        Panic();
        break;
    }

    return status;
}

/*********************************************************************/
void GSoundTargetBleRxComplete(GSoundChannelType type, const uint8_t *buffer,uint32_t len)
{
    UNUSED(type);
    UNUSED(buffer);
    UNUSED(len);
}

/*********************************************************************/
uint16_t GSoundTargetBleGetMtu(void)
{
    return GattGaaCommServerGetMtu();
}
