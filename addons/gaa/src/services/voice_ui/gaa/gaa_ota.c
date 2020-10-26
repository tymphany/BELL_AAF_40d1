/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Implementation for GAA OTA
*/


#include <string.h>
#include <stdlib.h>

#include <byte_utils.h>
#include <logging.h>
#include <upgrade.h>
#include <upgrade_msg_host.h>
#include "gsound_target_info.h"
#include "gsound_target_ota.h"
#include "gaa_private.h"
#include "device_upgrade.h"
#include "gaa_pmalloc_pools.h"
#include "device_upgrade_peer.h"
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
#include <handset_service.h>
#include <bt_device.h>
#include "gsound_target_bt.h"
#include "gatt_gaa_comm_server.h"
#include "voice_ui_container.h"
#endif

/* Define the base of the set of internal GAA OTA messages */
#define GAA_OTA_MSG_BASE   0x6000

/* The size of the id and length fields at the start of a message to be sent */
#define ID_AND_LENGTH_SIZE  (sizeof(uint8) + sizeof(uint16))

/* The in progess identifier for GAA OTA */
#define GAA_OTA_IN_PROGRESS_ID 0x426f7461

/*
 * Define the delay in millisectonds to be used for the GAA OTA rewind timeout.
 */
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
/*
 * Define the delay in millisectonds to be used for the GAA OTA rewind timeout.
 */
#define GAA_OTA_REWIND_TIMEOUT      3000

/*
 * Define the delay in millisectonds to be used for the GAA OTA disconnect timeout.
 */
#define GAA_OTA_DISCONNECT_TIMEOUT  2000

#else 
#define GAA_OTA_TIMEOUT_DELAY  3000
#endif

/*
 * Define constants to make calls to UpgradeTransportConnectRequest more readable.
 */
#define DATA_CFM_IS_NOT_NEEDED          FALSE
#define DO_NOT_REQUEST_MULTIPLE_BLOCKS  FALSE

/* The GAA OTA entity states */
typedef enum {
    GAA_OTA_STATE_READY,
    GAA_OTA_STATE_BEGINNING_CONNECT,
    GAA_OTA_STATE_BEGINNING_SYNC,
    GAA_OTA_STATE_BEGINNING_START,
    GAA_OTA_STATE_AWAITING_DATA_REQ,
    GAA_OTA_STATE_DATA,
    GAA_OTA_STATE_APPLY,
    GAA_OTA_STATE_REBOOT_CONNECT,
    GAA_OTA_STATE_REBOOT_SYNC,
    GAA_OTA_STATE_REBOOT_START,
    GAA_OTA_STATE_REBOOT_COMMIT,
    GAA_OTA_STATE_ABORT
} GAA_OTA_STATE_T;

#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
/* The message types for GAA OTA entity internal messages. */
typedef enum {
    GAA_OTA_INTERNAL_REWIND = GAA_OTA_MSG_BASE,
    GAA_OTA_INTERNAL_DONE,
    GAA_OTA_INTERNAL_DISCONNECT
} _GAA_OTA_INTERNAL_MSG_T;

#else
/* The message types for GAA OTA entity internal messages. */
typedef enum {
    GAA_OTA_INTERNAL_ERROR = GAA_OTA_MSG_BASE,
    GAA_OTA_INTERNAL_TIMEOUT,
    GAA_OTA_INTERNAL_OTA_DONE
} _GAA_OTA_INTERNAL_MSG_T;
#endif
/* The structre for the GAA OTA entity data. */
typedef struct
{
    /* The task data for the message handler we are going to register with the upgrade library. */
    TaskData task_data;
    /* The error callback function provided by GSoundTargetOtaInit. */
    GSoundStatus (*gsound_target_on_ota_error)(GSoundStatus status_code);
    /* The rewind callback function provided by GSoundTargetOtaInit. */
    GSoundStatus (*gsound_target_on_rewind)(uint32_t resume_offset);
    /* The OTA done callback function provided by GSoundTargetOtaInit. */
    GSoundStatus (*gsound_target_on_ota_done)(void);
    /* The total_size value supplied by GSoundTargetOtaBegin. */
    int32_t total_size;
    /* The device_index value supplied by GSoundTargetOtaBegin. */
    uint32_t device_index;
    /* The pointer to OTA data buffer for GSoundTargetOtaData. */
    uint8 *data;
    /* The start index to the OTA data buffer for GSoundTargetOtaData. */
    uint16 start;
    /* The end index to the OTA data buffer for GSoundTargetOtaData. */
    uint16 end;
    /* The offset within the total_size expected stream corresponding to the start field value */
    int32_t start_offset;
    /* The offset within the total_size expected stream corresponding to the end field value */
    int32_t end_offset;
    /* The number of bytes requested in the last UPGRADE_HOST_DATA_BYTES_REQ message. */
    uint32 num_bytes;
    /* The offest to the start of the next data requested in the last UPGRADE_HOST_DATA_BYTES_REQ message. */
    uint32 upgrade_start_offset;
    /* The current state of the GAA OTA state machine. */
    GAA_OTA_STATE_T state;
    /* A boolean indicating whether or not a a call to gsound_target_on_rewind is outstanding */
    bool rewind_outstanding;
    /*
     *  When rewind_outstanding is TRUE, the value in GSoundTargetOtaData byte_offset we are
     *  looking for in order to continue handling data.
     */
    uint32_t resume_offset;
} GAA_OTA_T;

static GAA_OTA_T *gaa_ota = NULL;

/*
 * A module static variable used to track whether the last reboot was for a 
 * Gaa upgrade. This is set by the gaa_OtaRebootDueToUpgrade() function
 * and used for the GaaRebootDueToOtaUpgrade function because the
 * GSoundTargetOtaInit function calls the appUpgradeSetContext function with
 * APP_UPGRADE_CONTEXT_UNUSED to clear the context once
 * APP_UPGRADE_CONTEXT_GAA_OTA has been read.
 */
static bool gaa_ota_reboot_occurred = FALSE;

/********************************************************************
* Local functions prototypes:
*/
static void gaa_OtaFreeDataBuffer(void);
static void gaa_OtaCancelPendingRewind(void);
static bool gaa_OtaRebootDueToUpgrade(void);
static void gaa_OtaSetState(GAA_OTA_STATE_T state);
static void gaa_OtaSendError(GSoundStatus status);
static uint16 gaa_Get2Bytes(const uint8 *src, uint16 byte_index, uint16 *val);
static uint16 gaa_Get4Bytes(const uint8 *src, uint16 byte_index, uint32 *val);
static void gaa_OtaRewind(uint32_t resume_offset);
static GSoundStatus gaa_OtaCreate(const GSoundOtaInterface *handler);
static GSoundStatus gaa_OtaCreateData(int32_t total_size, uint32_t device_index);
static void gaa_OtaAppendData(const uint8_t *data, size_t length, uint32_t byte_offset, size_t space);
static uint16 gaa_OtaGetDataFromStart(uint16 byte_index, uint16 length, uint8 *payload);

/* Incoming message handling functions */
static void gaa_OtaMessageHandler(Task task, MessageId id, Message message);
static void gaa_OtaHandleRewind(void);
static void gaa_OtaHandleOtaDone(void);
static void gaa_OtaHandleUpgradeTransportConnectCfm(UPGRADE_TRANSPORT_CONNECT_CFM_T *msg);
static void gaa_OtaHandleUpgradeTransportDataInd(UPGRADE_TRANSPORT_DATA_IND_T *msg);
static void gaa_OtaHandleUpgradeTransportDisconnectCfm(UPGRADE_TRANSPORT_DISCONNECT_CFM_T *msg);
static void gaa_OtaHandleDeviceUpgradePeerStarted(void);
static void gaa_OtaHandleUpgradeHostSyncCfm(uint16 size_data, uint8 *data);
static void gaa_OtaHandleUpgradeStartCfm(uint16 size_data, uint8 *data);
static void gaa_OtaHandleUpgradeHostCommitReq(uint16 size_data, uint8 *data);
static void gaa_OtaHandleUpgradeHostErrorWarnInd(uint16 size_data, uint8 *data);
static void gaa_OtaUpdateUpgradeHostData(UPGRADE_HOST_DATA_BYTES_REQ_T *msg);
static void gaa_OtaHandleUpgradeHostDataBytesReq(uint16 size_data, uint8 *data);
static void gaa_OtaHandleUpgradeHostIsCsrValidDoneCfm(uint16 size_data, uint8 *data);
static void gaa_OtaHandleUpgradeHostTransferCompleteInd(void);
static void gaa_OtaHandleUpgradeHostAbortCfm(void);

static GSoundStatus gaa_OtaTranslateUpgradeHostErrorCode(UpgradeHostErrorCode error_code);
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
static void gaa_OtaHandleOtaDisconnect(void);
#endif

/* Outgoing message handling functions */
static void gaa_OtaSendShortMessage(UpgradeMsgHost id);
static void gaa_OtaSendUpgradeHostSyncReq(uint32 in_progress_id);
static void gaa_OtaSendUpgradeHostData(void);
static void gaa_OtaSendUpgradeHostGenericAction(uint16 msgId, uint16 action);
static void gaa_OtaSendUpgradeHostCommitCfm(UpgradeHostActionCode action);
static void gaa_OtaSendUpgradeHostTransferCompleteRes(uint16 action);
static void gaa_OtaSendUpgradeHostInProgressRes(uint16 action);
static void gaa_OtaSendUpgradeHostErrorWarnRsp(uint16 error_code);

#ifdef HOSTED_TEST_ENVIRONMENT
const TaskData gaa_ota_test_task = {gaa_OtaMessageHandler};
#endif

static void gaa_OtaFreeDataBuffer(void)
{
    if (gaa_ota->data)
    {
        free(gaa_ota->data);
        gaa_ota->data = NULL;
    }
}

static void gaa_OtaCancelPendingRewind(void)
{
    if (gaa_ota->rewind_outstanding)
    {
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
        MessageCancelAll(&gaa_ota->task_data, GAA_OTA_INTERNAL_REWIND);
#else    	
        MessageCancelAll(&gaa_ota->task_data, GAA_OTA_INTERNAL_TIMEOUT);
#endif        
        gaa_ota->rewind_outstanding = FALSE;
    }
}

static bool gaa_OtaRebootDueToUpgrade(void)
{
    app_upgrade_context_t context = appUpgradeGetContext();
    DEBUG_LOG("gaa_OtaRebootDueToUpgrade %d", (context == APP_UPGRADE_CONTEXT_GAA_OTA));
    return (context == APP_UPGRADE_CONTEXT_GAA_OTA);
}

static uint16 gaa_Get2Bytes(const uint8 *src, uint16 byte_index, uint16 *val)
{
    *val =  ((uint16) src[byte_index] << 8);
    *val |= (uint16) src[byte_index + 1];
    return 2;
}

static uint16 gaa_Get4Bytes(const uint8 *src, uint16 byte_index, uint32 *val)
{
    *val =  ((uint32) src[byte_index] << 24);
    *val |= ((uint32) src[byte_index + 1] << 16);
    *val |= ((uint32) src[byte_index + 2] << 8);
    *val |= (uint32) src[byte_index + 3];
    return 4;
}

static void gaa_OtaSetState(GAA_OTA_STATE_T state)
{
    if (gaa_ota->state != state)
    {
        gaa_ota->state = state;
    }
}

static void gaa_OtaRewind(uint32_t resume_offset)
{
    if (!gaa_ota->rewind_outstanding)
    {
        gaa_ota->rewind_outstanding = TRUE;
        gaa_ota->resume_offset = resume_offset;
        DEBUG_LOG("gaa_OtaRewind rewind to %ld", resume_offset);
    }
}

static void gaa_OtaSendError(GSoundStatus status)
{ 
    DEBUG_LOG("gaa_OtaSendError %u", status);
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
    if (gaa_ota->state != GAA_OTA_STATE_APPLY)
    {
        gaa_ota->gsound_target_on_ota_error(status);
        
        /* If the error is due to a disconnection, then the control channel 
         * may be lost before GSound has time to send its Abort message in response 
         * to the error. Clean-up here in case this happens */
        gaa_OtaCancelPendingRewind();
        gaa_OtaFreeDataBuffer();
        gaa_OtaSetState(GAA_OTA_STATE_READY);
        appUpgradeSetContext(APP_UPGRADE_CONTEXT_UNUSED);
        UpgradeTransportDisconnectRequest();
    }
    else
    {
        /* We have disconnected, or we are in the process of disconnecting, so reboot on error */
        DEBUG_LOG("gaa_OtaSendError: rebooting to reconnect");
        VoiceUi_RebootLater();
    }
#else    
    gaa_ota->gsound_target_on_ota_error(status);
    
    /* If the error is due to a disconnection, then the control channel 
     * may be lost before GSound has time to send its Abort message in response 
     * to the error. Clean-up here in case this happens */
    gaa_OtaCancelPendingRewind();
    gaa_OtaFreeDataBuffer();
#endif
}

static void gaa_OtaSendUpgradeHostSyncReq(uint32 in_progress_id)
{
    uint16 byte_index = 0;
    uint8 *payload = malloc(sizeof(UPGRADE_HOST_SYNC_REQ_T) + ID_AND_LENGTH_SIZE);
    if (payload == NULL)
    {
        gaa_OtaSendError(GSOUND_STATUS_OUT_OF_MEMORY);
    }
    else
    {
        byte_index += ByteUtilsSet1Byte(payload, byte_index, UPGRADE_HOST_SYNC_REQ - UPGRADE_HOST_MSG_BASE);
        byte_index += ByteUtilsSet2Bytes(payload, byte_index, (uint16) sizeof(UPGRADE_HOST_SYNC_REQ_T));
        byte_index += ByteUtilsSet4Bytes(payload, byte_index, in_progress_id);
        UpgradeProcessDataRequest(byte_index, payload);
        free(payload);
    }
}

static uint16 gaa_OtaGetDataFromStart(uint16 byte_index, uint16 length, uint8 *payload)
{
    if ((gaa_ota->start + length) < GAA_OTA_DATA_BUFFER_SIZE)
    {
        /* All the required memory is in a contigous block in gaa_ota->data */
        memcpy(&payload[byte_index], &gaa_ota->data[gaa_ota->start], length);
        byte_index += length;
    }
    else
    {
        /* The required data is split between the end and the start in gaa_ota->data */
        uint16 count = GAA_OTA_DATA_BUFFER_SIZE - gaa_ota->start;
        memcpy(&payload[byte_index], &gaa_ota->data[gaa_ota->start], count);
        byte_index += count;
        memcpy(&payload[byte_index], &gaa_ota->data[0], length - count);
        byte_index += (length - count);
    }
    gaa_ota->start_offset += length;
    gaa_ota->start += length;
    gaa_ota->start %= GAA_OTA_DATA_BUFFER_SIZE;
    return byte_index;
}

static void gaa_OtaSendUpgradeHostData(void)
{
    if (gaa_ota->state == GAA_OTA_STATE_DATA)
    {
        /* We have received a UPGRADE_HOST_DATA_BYTES_REQ */
        if ((gaa_ota->end_offset - gaa_ota->start_offset) > 0)
        {
            /* There is unsent data in the buffer */
            if ((gaa_ota->upgrade_start_offset == 0) || (gaa_ota->upgrade_start_offset == gaa_ota->start_offset))
            {
                /*
                 * The upgrade library has not reqested data from a particular location,
                 * or it has and that location has been reached.
                 */
                uint16 length = ((gaa_ota->end_offset - gaa_ota->start_offset) > gaa_ota->num_bytes) ?
                    gaa_ota->num_bytes : (gaa_ota->end_offset - gaa_ota->start_offset);
                uint16 byte_index = 0;
                bool last_packet = ((gaa_ota->start_offset + length) == gaa_ota->total_size);
                if (last_packet || (length == gaa_ota->num_bytes))
                {
                    /* We have either the last packet, or enough to fill packet with num_bytes */
                    uint8 *payload = malloc(sizeof(UPGRADE_HOST_DATA_T) + (length - 2) + 1);
                    if (payload == NULL)
                    {
                        gaa_OtaSendError(GSOUND_STATUS_OUT_OF_MEMORY);
                    }
                    else
                    {
                        byte_index += ByteUtilsSet1Byte(payload, byte_index, UPGRADE_HOST_DATA - UPGRADE_HOST_MSG_BASE);
                        /* Set length in the payload to one for the last_packet flag, plus the length of the data */
                        byte_index += ByteUtilsSet2Bytes(payload, byte_index, length + 1);
                        byte_index += ByteUtilsSet1Byte(payload, byte_index, last_packet);
                        byte_index = gaa_OtaGetDataFromStart(byte_index, length, payload);
                        if (!last_packet)
                        {
                            /* Don't send another until next UPGRADE_HOST_DATA_BYTES_REQ received. */
                            gaa_OtaSetState(GAA_OTA_STATE_AWAITING_DATA_REQ);
                        }
                        else
                        {
                            DEBUG_LOG("gaa_OtaSendUpgradeHostData: Last packet");
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
 							gaa_OtaFreeDataBuffer();
                            MessageSend(&gaa_ota->task_data, GAA_OTA_INTERNAL_DONE, NULL);                        
#else
                            MessageSend(&gaa_ota->task_data, GAA_OTA_INTERNAL_OTA_DONE, NULL);
#endif                            
                        }
                        UpgradeProcessDataRequest(byte_index, payload);
                        free(payload);
                    }
                }
            }
        }
    }
}

static void gaa_OtaSendUpgradeHostGenericAction(uint16 msg_id, uint16 action)
{
    uint16 byte_index = 0;
    uint8 *payload = malloc(sizeof(UPGRADE_HOST_GENERIC_ACTION_T) + ID_AND_LENGTH_SIZE);
    if (payload == NULL)
    {
        gaa_OtaSendError(GSOUND_STATUS_OUT_OF_MEMORY);
    }
    else
    {
        byte_index += ByteUtilsSet1Byte(payload, byte_index, msg_id - UPGRADE_HOST_MSG_BASE);
        byte_index += ByteUtilsSet2Bytes(payload, byte_index, (uint16) sizeof(UPGRADE_HOST_GENERIC_ACTION_T));
        byte_index += ByteUtilsSet2Bytes(payload, byte_index, action);
        UpgradeProcessDataRequest(byte_index, payload);
        free(payload);
    }
}

static void gaa_OtaSendUpgradeHostCommitCfm(UpgradeHostActionCode action)
{
    gaa_OtaSendUpgradeHostGenericAction(UPGRADE_HOST_COMMIT_CFM, action);
}

static void gaa_OtaSendUpgradeHostTransferCompleteRes(uint16 action)
{
    gaa_OtaSendUpgradeHostGenericAction(UPGRADE_HOST_TRANSFER_COMPLETE_RES, action);
}

static void gaa_OtaSendUpgradeHostInProgressRes(uint16 action)
{
    gaa_OtaSendUpgradeHostGenericAction(UPGRADE_HOST_IN_PROGRESS_RES, action);
}

static void gaa_OtaSendUpgradeHostErrorWarnRsp(uint16 error_code)
{
    gaa_OtaSendUpgradeHostGenericAction(UPGRADE_HOST_ERRORWARN_RES, error_code);
}

static void gaa_OtaSendShortMessage(UpgradeMsgHost id)
{
    uint16 byte_index = 0;
    uint8 *payload = malloc(ID_AND_LENGTH_SIZE);
    if (payload == NULL)
    {
        gaa_OtaSendError(GSOUND_STATUS_OUT_OF_MEMORY);
    }
    else
    {
        byte_index += ByteUtilsSet1Byte(payload, byte_index, id - UPGRADE_HOST_MSG_BASE);
        byte_index += ByteUtilsSet2Bytes(payload, byte_index, 0);
        UpgradeProcessDataRequest(byte_index, payload);
        free(payload);
    }
}

static void gaa_OtaHandleRewind(void)
{
    if (gaa_ota->rewind_outstanding)
    {
        /* Not already rewound, so request rewind */
        GSoundStatus status = gaa_ota->gsound_target_on_rewind(gaa_ota->resume_offset);
        if (status == GSOUND_STATUS_OK)
        {
            /*
             * Start the timeout to check that get rewound within the expected time.
             * If timeout then will request the rewind again.
             */
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
			MessageSendLater(&gaa_ota->task_data, GAA_OTA_INTERNAL_REWIND, NULL, GAA_OTA_REWIND_TIMEOUT);
#else             
            MessageSendLater(&gaa_ota->task_data, GAA_OTA_INTERNAL_TIMEOUT, NULL, GAA_OTA_TIMEOUT_DELAY);
#endif            
            DEBUG_LOG("gaa_OtaHandleRewind async rewind to %lu", gaa_ota->resume_offset);
        }
        else
        {
            DEBUG_LOG("gaa_OtaHandleRewind failed %d", status);
        }
    }
}

static void gaa_OtaHandleOtaDone(void)
{
    DEBUG_LOG("gaa_OtaHandleOtaDone");
    gaa_ota->gsound_target_on_ota_done();
}
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
static void gaa_OtaHandleOtaDisconnect(void)
{
    DEBUG_LOG("gaa_OtaHandleOtaDisconnect");
    bdaddr handset_addr;
    GSoundTargetBtChannelClose(GSOUND_CHANNEL_CONTROL);
    GSoundTargetBtChannelClose(GSOUND_CHANNEL_AUDIO);
    appDeviceGetHandsetBdAddr(&handset_addr);
    HandsetService_SetBleConnectable(FALSE);
    HandsetService_DisconnectRequest(&gaa_ota->task_data, &handset_addr);
    GattGaaCommDisconnect();
}
#endif
static void gaa_OtaHandleUpgradeHostSyncCfm(uint16 size_data, uint8 *data)
{
    UNUSED(size_data);
    uint16 byte_index = 0;
    uint8 id;
    uint16 length;
    UPGRADE_HOST_SYNC_CFM_T *msg = (UPGRADE_HOST_SYNC_CFM_T *)malloc(sizeof(UPGRADE_HOST_SYNC_CFM_T));
    if (!msg)
    {
        gaa_OtaSendError(GSOUND_STATUS_OUT_OF_MEMORY);
    }
    else
    {
        byte_index += ByteUtilsGet1Byte(data, byte_index, &id);
        byte_index += gaa_Get2Bytes(data, byte_index, &length);
        byte_index += ByteUtilsGet1Byte(data, byte_index, &msg->upgradeState);
        byte_index += gaa_Get4Bytes(data, byte_index, &msg->inProgressId);
        byte_index += ByteUtilsGet1Byte(data, byte_index, &msg->version);
        switch(gaa_ota->state)
        {
            case GAA_OTA_STATE_REBOOT_SYNC:
                /*
                 * Could check the upgradeState, which should be a value from the
                 * UpdateResumePoint enumeration. It ought to have a value of
                 * UPGRADE_RESUME_POINT_COMMIT (4). But what to do if not that
                 * value? So for now, ignore the value of upgradeState, and also
                 * ignore the values of in_progress_id and version.
                 */
                DEBUG_LOG("gaa_OtaHandleUpgradeHostSyncCfm: GAA_OTA_STATE_REBOOT_SYNC");
                gaa_OtaSetState(GAA_OTA_STATE_REBOOT_START);
                gaa_OtaSendShortMessage(UPGRADE_HOST_START_REQ);
                break;

            case GAA_OTA_STATE_BEGINNING_SYNC:
                /*
                 * Could check the upgradeState, which should be a value from the
                 * UpdateResumePoint enumeration. It ought to have a value of
                 * UPGRADE_RESUME_POINT_START (0). But what to do if not that
                 * value? So for now, ignore the value of upgradeState, and also
                 * ignore the values of in_progress_id and version.
                 */
                gaa_OtaSetState(GAA_OTA_STATE_BEGINNING_START);
                gaa_OtaSendShortMessage(UPGRADE_HOST_START_REQ);
                break;

            default:
                DEBUG_LOG("gaa_OtaHandleUpgradeHostSyncCfm unexpected state %d", gaa_ota->state);
                /* Ignore the UPGRADE_HOST_SYNC_CFM message received in an unexpected state. */
                break;
        }
        free(msg);
    }
}

static void gaa_OtaHandleUpgradeStartCfm(uint16 size_data, uint8 *data)
{
    UNUSED(size_data);
    uint16 byte_index = 0;
    uint8 id;
    uint16 length;
    UPGRADE_HOST_START_CFM_T *msg = (UPGRADE_HOST_START_CFM_T *)malloc(sizeof(UPGRADE_HOST_START_CFM_T));
    if (!msg)
    {
        gaa_OtaSendError(GSOUND_STATUS_OUT_OF_MEMORY);
    }
    else
    {
        byte_index += ByteUtilsGet1Byte(data, byte_index, &id);
        byte_index += gaa_Get2Bytes(data, byte_index, &length);
        byte_index += ByteUtilsGet1Byte(data, byte_index, &msg->status);
        byte_index += gaa_Get2Bytes(data, byte_index, &msg->batteryLevel);
        /*
         * The upgrade library always sends status of zero and battery level of
         * 0x666 at the moment, so no point in doing anything with these values.
         */
        switch(gaa_ota->state)
        {
            case GAA_OTA_STATE_REBOOT_START:
                DEBUG_LOG("gaa_OtaHandleUpgradeStartCfm: GAA_OTA_STATE_REBOOT_START");
                gaa_OtaSetState(GAA_OTA_STATE_REBOOT_COMMIT);
                /* We expect to receive a subsequent UPGRADE_HOST_COMMIT_REQ. */
                gaa_OtaSendUpgradeHostInProgressRes(0);
                break;

            case GAA_OTA_STATE_BEGINNING_START:
                gaa_OtaSetState(GAA_OTA_STATE_AWAITING_DATA_REQ);
                gaa_OtaSendShortMessage(UPGRADE_HOST_START_DATA_REQ);
                break;

            default:
                DEBUG_LOG("gaa_OtaHandleUpgradeStartCfm unexpected state: %d", gaa_ota->state);
                /* Ignore the UPGRADE_HOST_START_CFM message received in an unexpected state. */
                break;
        }
        free(msg);
    }
}

static void gaa_OtaHandleUpgradeHostCommitReq(uint16 size_data, uint8 *data)
{
    UNUSED(size_data);
    UNUSED(data);
    switch(gaa_ota->state)
    {
        case GAA_OTA_STATE_REBOOT_COMMIT:
            DEBUG_LOG("gaa_OtaHandleUpgradeHostCommitReq: GAA_OTA_STATE_REBOOT_COMMIT");
            gaa_OtaSendUpgradeHostCommitCfm(UPGRADE_HOSTACTION_YES);
            gaa_OtaSetState(GAA_OTA_STATE_READY);
            UpgradeTransportDisconnectRequest();
            break;

        default:
            DEBUG_LOG("gaa_OtaHandleUpgradeHostCommitReq unexpected state %d", gaa_ota->state);
            /* Ignore the UPGRADE_HOST_COMMIT_REQ message received in an unexpected state. */
            break;
    }
}

static GSoundStatus gaa_OtaTranslateUpgradeHostErrorCode(UpgradeHostErrorCode error_code)
{
    GSoundStatus error = GSOUND_STATUS_ERROR;
    switch (error_code)
    {
        case UPGRADE_HOST_ERROR_NO_MEMORY:
        case UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_MEMORY:
            error = GSOUND_STATUS_OUT_OF_MEMORY;
            break;

        default:
            break;
    }

    return error;
}

static void gaa_OtaHandleUpgradeHostErrorWarnInd(uint16 size_data, uint8 *data)
{
    UNUSED(size_data);
    uint16 byte_index = 0;
    uint8 id;
    uint16 length;
    UPGRADE_HOST_ERRORWARN_IND_T *msg = (UPGRADE_HOST_ERRORWARN_IND_T *)malloc(sizeof(UPGRADE_HOST_ERRORWARN_IND_T));
    if (!msg)
    {
        gaa_OtaSendError(GSOUND_STATUS_OUT_OF_MEMORY);
    }
    else
    {
        byte_index += ByteUtilsGet1Byte(data, byte_index, &id);
        byte_index += gaa_Get2Bytes(data, byte_index, &length);
        byte_index += gaa_Get2Bytes(data, byte_index, &msg->errorCode);
        DEBUG_LOG("gaa_OtaHandleUpgradeHostErrorWarnInd 0x%04x", msg->errorCode);
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
        switch(gaa_ota->state)
        {
            case GAA_OTA_STATE_REBOOT_CONNECT:
            case GAA_OTA_STATE_REBOOT_SYNC:
            case GAA_OTA_STATE_REBOOT_START:
            case GAA_OTA_STATE_REBOOT_COMMIT:
                //add case 04866605 patch
                //gaa_OtaSetState(GAA_OTA_STATE_READY);                
                gaa_OtaSetState(GAA_OTA_STATE_ABORT);
                DEBUG_LOG("Setting OTA state to GAA_OTA_STATE_ABORT");
                break;

            default:
                break;
        }
        /*
         * Send an indication to the GSound library according to the
         * error_code and expect it to call GSoundTargetOtaAbort if it
         * needs to.
         */
        gaa_OtaSendError(gaa_OtaTranslateUpgradeHostErrorCode((UpgradeHostErrorCode) msg->errorCode));        
#else
        if (msg->errorCode >= UPGRADE_HOST_ERROR_INTERNAL_ERROR_DEPRECATED)
        {
            /*
             * UPGRADE_HOST_SUCCESS and UPGRADE_HOST_OEM_VALIDATION_SUCCESS are not
             * indications that an error has occured. Only those UPGRADE_HOST_ERROR_...
             * from UPGRADE_HOST_ERROR_INTERNAL_ERROR_DEPRECATED upwards are
             * indicative of an error.
             */
            switch(gaa_ota->state)
            {
                case GAA_OTA_STATE_REBOOT_CONNECT:
                case GAA_OTA_STATE_REBOOT_SYNC:
                case GAA_OTA_STATE_REBOOT_START:
                case GAA_OTA_STATE_REBOOT_COMMIT:
                    gaa_OtaSetState(GAA_OTA_STATE_READY);
                    break;

                default:
                    break;
            }
            /*
             * Send an indication to the GSound library according to the
             * error_code and expect it to call GSoundTargetOtaAbort if it
             * needs to.
             */
            gaa_OtaSendError(gaa_OtaTranslateUpgradeHostErrorCode((UpgradeHostErrorCode) msg->errorCode));
        }
#endif
        gaa_OtaSendUpgradeHostErrorWarnRsp(msg->errorCode);
        //add case 04866605 patch
        gaa_OtaSendShortMessage(UPGRADE_HOST_ABORT_REQ);
        free(msg);
    }
}

static void gaa_OtaUpdateUpgradeHostData(UPGRADE_HOST_DATA_BYTES_REQ_T *msg)
{
    gaa_ota->num_bytes = msg->numBytes;
    if (msg->startOffset > gaa_ota->start_offset)
    {
        /* Have not already passed the required start offset */
        if (msg->startOffset > gaa_ota->end_offset)
        {
            /* The required start offset is not already in the gaa_ota->data buffer */
            gaa_ota->upgrade_start_offset = msg->startOffset;
            DEBUG_LOG("gaa_OtaUpdateUpgradeHostData upgrade start offset %lu", gaa_ota->upgrade_start_offset);
        }
        else
        {
            /* The required start offset is already in the gaa_ota->data buffer */
            uint16 length = msg->startOffset - gaa_ota->start_offset;
            gaa_ota->start_offset += length;
            gaa_ota->start += length;
            gaa_ota->start %= GAA_OTA_DATA_BUFFER_SIZE;
        }
    }
    if ((gaa_ota->end_offset - gaa_ota->start_offset) != 0)
    {
        gaa_OtaSendUpgradeHostData();
    }
}

static void gaa_OtaHandleUpgradeHostDataBytesReq(uint16 size_data, uint8 *data)
{
    uint16 byte_index = 0;
    uint8 id;
    uint16 length;
    UPGRADE_HOST_DATA_BYTES_REQ_T *msg = (UPGRADE_HOST_DATA_BYTES_REQ_T *)malloc(sizeof(UPGRADE_HOST_DATA_BYTES_REQ_T));
    if (!msg)
    {
        gaa_OtaSendError(GSOUND_STATUS_OUT_OF_MEMORY);
    }
    else
    {
        byte_index += ByteUtilsGet1Byte(data, byte_index, &id);
        byte_index += gaa_Get2Bytes(data, byte_index, &length);
        byte_index += gaa_Get4Bytes(data, byte_index, &msg->numBytes);
        byte_index += gaa_Get4Bytes(data, byte_index, &msg->startOffset);
        UNUSED(size_data);

        switch(gaa_ota->state)
        {
            case GAA_OTA_STATE_AWAITING_DATA_REQ:
                gaa_OtaSetState(GAA_OTA_STATE_DATA);
                gaa_OtaUpdateUpgradeHostData(msg);
                break;
            case GAA_OTA_STATE_DATA:
                gaa_OtaUpdateUpgradeHostData(msg);
                break;

            default:
                DEBUG_LOG("gaa_OtaHandleUpgradeHostDataBytesReq unexpected state %d", gaa_ota->state);
                /* Ignore the UPGRADE_HOST_DATA_BYTES_REQ message received in an unexpected state. */
                break;
        }
        free(msg);
    }
}

static void gaa_OtaHandleUpgradeHostIsCsrValidDoneCfm(uint16 size_data, uint8 *data)
{
    uint16 byte_index = 0;
    uint8 id;
    uint16 length;
    UPGRADE_HOST_IS_CSR_VALID_DONE_CFM_T *msg = (UPGRADE_HOST_IS_CSR_VALID_DONE_CFM_T *)malloc(sizeof(UPGRADE_HOST_IS_CSR_VALID_DONE_CFM_T));
    if (!msg)
    {
        gaa_OtaSendError(GSOUND_STATUS_OUT_OF_MEMORY);
    }
    else
    {
        byte_index += ByteUtilsGet1Byte(data, byte_index, &id);
        byte_index += gaa_Get2Bytes(data, byte_index, &length);
        byte_index += gaa_Get2Bytes(data, byte_index, &msg->backOffTime);
        UNUSED(size_data);
        MessageSendLater(&gaa_ota->task_data, UPGRADE_HOST_IS_CSR_VALID_DONE_REQ, NULL, msg->backOffTime);
        free(msg);
    }
}

static void gaa_OtaHandleUpgradeHostTransferCompleteInd(void)
{
    /* Set the upgrade transport type so we can tell we caused the upgrade after reboot. */
    appUpgradeSetContext(APP_UPGRADE_CONTEXT_GAA_OTA);
    /* Send the UPGRADE_TRANSFER_COMPLETE_RES message to cause the upgrade to continue (action 0). */
    gaa_OtaSendUpgradeHostTransferCompleteRes(0);
}

static void gaa_OtaHandleUpgradeHostAbortCfm(void)
{
    if (gaa_ota->state == GAA_OTA_STATE_ABORT)
    {
        gaa_OtaSetState(GAA_OTA_STATE_READY);
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
        gaa_OtaFreeDataBuffer();
#else        
        if (gaa_ota->data)
        {
            free(gaa_ota->data);
            gaa_ota->data = NULL;
        }
#endif        
        UpgradeTransportDisconnectRequest();
    }
    else if (gaa_ota->state == GAA_OTA_STATE_BEGINNING_CONNECT)
    {
        gaa_OtaSetState(GAA_OTA_STATE_BEGINNING_SYNC);
        gaa_OtaSendUpgradeHostSyncReq(GAA_OTA_IN_PROGRESS_ID);
    }
    else
    {
        DEBUG_LOG("gaa_OtaHandleUpgradeHostAbortCfm state %u", gaa_ota->state);
    }
}

static void gaa_OtaHandleUpgradeTransportConnectCfm(UPGRADE_TRANSPORT_CONNECT_CFM_T *msg)
{
    switch(gaa_ota->state)
    {
        case GAA_OTA_STATE_REBOOT_CONNECT:
            if (msg->status == upgrade_status_success)
            {
                DEBUG_LOG("gaa_OtaHandleUpgradeTransportConnectCfm: GAA_OTA_STATE_REBOOT_CONNECT: success");
                UpgradePermit(upgrade_perm_assume_yes);
                gaa_OtaSetState(GAA_OTA_STATE_REBOOT_SYNC);
                gaa_OtaSendUpgradeHostSyncReq(GAA_OTA_IN_PROGRESS_ID);
            }
            else
            {
                DEBUG_LOG("gaa_OtaHandleUpgradeTransportConnectCfm: GAA_OTA_STATE_REBOOT_CONNECT: failure");
                gaa_OtaSetState(GAA_OTA_STATE_READY);
                UpgradeTransportDisconnectRequest();
                gaa_OtaSendError(GSOUND_STATUS_ERROR);
            }
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
			/* The reconnection to the upgrade library after OTA reboot has completed */
            gaa_ota_reboot_occurred = FALSE;
#endif
            break;

        case GAA_OTA_STATE_BEGINNING_CONNECT:
            if (msg->status == upgrade_status_success)
            {
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
                UpgradePeerSetDeviceRolePrimary(TRUE);
#endif
                UpgradePermit(upgrade_perm_assume_yes);
                gaa_OtaSetState(GAA_OTA_STATE_BEGINNING_SYNC);
                gaa_OtaSendUpgradeHostSyncReq(GAA_OTA_IN_PROGRESS_ID);
            }
            else
            {
                gaa_OtaSetState(GAA_OTA_STATE_READY);
                UpgradeTransportDisconnectRequest();
                gaa_OtaSendError(GSOUND_STATUS_ERROR);
            }
            break;

        default:
            DEBUG_LOG("gaa_OtaHandleUpgradeTransportConnectCfm unexpected state %d", gaa_ota->state);
            /* Ignore the UPGRADE_TRANSPORT_CONNECT_CFM message received in an unexpected state. */
            break;
    }
}

static void gaa_OtaHandleUpgradeTransportDataInd(UPGRADE_TRANSPORT_DATA_IND_T *msg)
{
    switch(msg->data[0] + UPGRADE_HOST_MSG_BASE)
    {
        case UPGRADE_HOST_SYNC_CFM:
            gaa_OtaHandleUpgradeHostSyncCfm(msg->size_data, msg->data);
            break;

        case UPGRADE_HOST_START_CFM:
            gaa_OtaHandleUpgradeStartCfm(msg->size_data, msg->data);
            break;

        case UPGRADE_HOST_COMMIT_REQ:
            gaa_OtaHandleUpgradeHostCommitReq(msg->size_data, msg->data);
            break;

        case UPGRADE_HOST_ERRORWARN_IND:
            DEBUG_LOG("gaa_OtaHandleUpgradeTransportDataInd rx UPGRADE_HOST_ERRORWARN_IND");
            gaa_OtaHandleUpgradeHostErrorWarnInd(msg->size_data, msg->data);
            break;

        case UPGRADE_HOST_DATA_BYTES_REQ:
            gaa_OtaHandleUpgradeHostDataBytesReq(msg->size_data, msg->data);
            break;

        case UPGRADE_HOST_IS_CSR_VALID_DONE_CFM:
            gaa_OtaHandleUpgradeHostIsCsrValidDoneCfm(msg->size_data, msg->data);
            break;

        case UPGRADE_HOST_TRANSFER_COMPLETE_IND:
            gaa_OtaHandleUpgradeHostTransferCompleteInd();
            break;

        case UPGRADE_HOST_ABORT_CFM:
            gaa_OtaHandleUpgradeHostAbortCfm();
            break;

        default:
            DEBUG_LOG("gaa_OtaHandleUpgradeTransportDataInd unexpected host msg 0x%04x", msg->data[0]);
            /* Ignore the unexpected message. */
            break;
    }
}

static void gaa_OtaHandleUpgradeTransportDisconnectCfm(UPGRADE_TRANSPORT_DISCONNECT_CFM_T *msg)
{
    UpgradePermit(upgrade_perm_always_ask);
    UNUSED(msg);
}

/*********************************************************************/

static void gaa_OtaHandleDeviceUpgradePeerStarted(void)
{
    if (gaa_ota_reboot_occurred)
    {
        /* The upgrade caused by GAA OTA. Start the connection sequence */
        DEBUG_LOG("gaa_OtaHandleDeviceUpgradePeerStarted");
        appUpgradeSetContext(APP_UPGRADE_CONTEXT_UNUSED);
        gaa_OtaSetState(GAA_OTA_STATE_REBOOT_CONNECT);
        UpgradeTransportConnectRequest(&gaa_ota->task_data,
            DATA_CFM_IS_NOT_NEEDED, DO_NOT_REQUEST_MULTIPLE_BLOCKS);
    }
    else
    {
        DEBUG_LOG("gaa_OtaHandleDeviceUpgradePeerStarted called without gaa_ota_reboot_occurred set");
    }
}

/*********************************************************************/

static void gaa_OtaMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
        /* Handle an external message from the upgrade library. */
        case UPGRADE_TRANSPORT_CONNECT_CFM:
            gaa_OtaHandleUpgradeTransportConnectCfm((UPGRADE_TRANSPORT_CONNECT_CFM_T *) message);
            break;

        case UPGRADE_TRANSPORT_DATA_IND:
            gaa_OtaHandleUpgradeTransportDataInd((UPGRADE_TRANSPORT_DATA_IND_T *) message);
            break;

        case UPGRADE_TRANSPORT_DISCONNECT_CFM:
            gaa_OtaHandleUpgradeTransportDisconnectCfm((UPGRADE_TRANSPORT_DISCONNECT_CFM_T *) message);
            break;

        /* Handle an external message from the upgrade peer. */
        case DEVICE_UPGRADE_PEER_INIT_CFM:
            /* Ignore */
            break;

        case DEVICE_UPGRADE_PEER_STARTED:
            gaa_OtaHandleDeviceUpgradePeerStarted();
            break;

        case DEVICE_UPGRADE_PEER_DISCONNECT:
            /* Ignore */
            break;
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
        /* Handle an external message from the handset service. */
        case HANDSET_SERVICE_DISCONNECT_CFM:
            /* Ignore */
            break;

        /* Handle an internal message. */
        case GAA_OTA_INTERNAL_REWIND:
            gaa_OtaHandleRewind();
            break;

        case GAA_OTA_INTERNAL_DONE:
            gaa_OtaHandleOtaDone();
            break;

        case GAA_OTA_INTERNAL_DISCONNECT:
            gaa_OtaHandleOtaDisconnect();
            break;

#else
        /* Handle an internal message. */
        case GAA_OTA_INTERNAL_TIMEOUT:
            gaa_OtaHandleRewind();
            break;

        case GAA_OTA_INTERNAL_OTA_DONE:
            gaa_OtaHandleOtaDone();
            break;

#endif
        default:
            DEBUG_LOG("gaa_OtaMessageHandler unexpected msg 0x%04x", id);
            /* Ignore the unexpected message. */
            break;
    }
}

static GSoundStatus gaa_OtaCreate(const GSoundOtaInterface *handler)
{
    GSoundStatus status = GSOUND_STATUS_OK;
    gaa_ota = malloc(sizeof(GAA_OTA_T));

    if (gaa_ota == NULL)
    {
        DEBUG_LOG("gaa_OtaCreate failed to allocate memory");
        status = GSOUND_STATUS_OUT_OF_MEMORY;
    }
    else
    {
        memset((void *) gaa_ota, 0, sizeof(GAA_OTA_T));

        gaa_ota->gsound_target_on_ota_error = handler->gsound_target_on_ota_error;
        gaa_ota->gsound_target_on_rewind = handler->gsound_target_on_rewind;
        gaa_ota->gsound_target_on_ota_done = handler->gsound_target_on_ota_done;
        gaa_ota->task_data.handler = gaa_OtaMessageHandler;
    }

    return status;
}

static GSoundStatus gaa_OtaCreateData(int32_t total_size, uint32_t device_index)
{
    GSoundStatus status = GSOUND_STATUS_OK;
    if (gaa_ota->data)
    {
        DEBUG_LOG("gaa_OtaCreateData memory has already been allocated");
    }
    else /* Allocate the memory for the buffer to be used in handling GSoundTargetOtaData. */
    {
        gaa_ota->data = (uint8 *)malloc(GAA_OTA_DATA_BUFFER_SIZE);
    }

    if (!gaa_ota->data)
    {
        DEBUG_LOG("gaa_OtaCreateData failed to allocate memory");
        status = GSOUND_STATUS_OUT_OF_MEMORY;
    }
    else
    {
        /* Initialise the fields used in accessing the gaa_ota->data array. */
        gaa_ota->start = 0;
        gaa_ota->end = 0;
        gaa_ota->start_offset = 0;
        gaa_ota->end_offset = 0;
        gaa_ota->num_bytes = 0;
        gaa_ota->upgrade_start_offset = 0;
        gaa_ota->total_size = total_size;
        gaa_ota->device_index = device_index;
        gaa_ota->rewind_outstanding = FALSE;
    }
    return status;
}

static void gaa_OtaAppendData(const uint8_t *data, size_t length, uint32_t byte_offset, size_t space)
{
    if (gaa_ota->upgrade_start_offset)
    {
        if (byte_offset + length < gaa_ota->upgrade_start_offset)
        {
            /* Have not reached the required data, yet. Ignore this data. */
            gaa_ota->end_offset = byte_offset + length;
            length = 0;
        }
        else
        {
            /* The required data has now been reached. Restart data handling. */
            gaa_ota->start = 0;
            gaa_ota->start_offset = gaa_ota->upgrade_start_offset;
            gaa_ota->end = 0;
            gaa_ota->end_offset = gaa_ota->start_offset;
            length -= (gaa_ota->upgrade_start_offset - byte_offset);
            gaa_ota->upgrade_start_offset = 0;
            DEBUG_LOG("gaa_OtaAppendData upgrade start offset %lu", gaa_ota->upgrade_start_offset);
        }
    }

    if (length > space)
    {
        /* We have been given more data than there is room for in the gaa_ota->data buffer. */
        length = space;
        if (length > 0)
        {
            gaa_OtaRewind(byte_offset + length);
        }
        else
        {
            gaa_OtaRewind(gaa_ota->end_offset);
        }
    }

    if (length > 0)
    {
        if (length <= (GAA_OTA_DATA_BUFFER_SIZE - gaa_ota->end))
        {
            /* There is contiguous room in gaa_ota->data for all the data to be copied. */
            memcpy(&gaa_ota->data[gaa_ota->end], data, length);
        }
        else
        {
            /* The area available in gaa_ota->data is split across the end and start of the gaa_ota->data buffer. */
            size_t count = GAA_OTA_DATA_BUFFER_SIZE - gaa_ota->end;
            memcpy(&gaa_ota->data[gaa_ota->end], data, count);
            memcpy(&gaa_ota->data[0], &data[count], length - count);
        }
        gaa_ota->end_offset += length;
        gaa_ota->end += length;
        gaa_ota->end %= GAA_OTA_DATA_BUFFER_SIZE;
    }
    if ((gaa_ota->state == GAA_OTA_STATE_DATA) && (gaa_ota->upgrade_start_offset == 0))
    {
        gaa_OtaSendUpgradeHostData();
    }
}

GSoundStatus GSoundTargetOtaInit(const GSoundOtaInterface *handler, GSoundOtaConfiguration *ota_device_config)
{
    GSoundStatus status = GSOUND_STATUS_OK;

    if (gaa_ota != NULL)
    {
        DEBUG_LOG("GSoundTargetOtaInit already initialised");
        status = GSOUND_STATUS_ERROR;
    }
    else if (handler == NULL)
    {
        DEBUG_LOG("GSoundTargetOtaInit handler parameter is NULL");
        status = GSOUND_STATUS_ERROR;
    }
    else if ((handler->gsound_target_on_ota_error == NULL) || (handler->gsound_target_on_rewind == NULL) || (handler->gsound_target_on_ota_done == NULL))
    {
        DEBUG_LOG("GSoundTargetOtaInit error %p and/or rewind %p NULL", handler->gsound_target_on_ota_error, handler->gsound_target_on_rewind);
        status = GSOUND_STATUS_ERROR;
    }
    else if (!UpgradeIsInitialised())
    {
        DEBUG_LOG("GSoundTargetOtaInit upgrade library has not been initialised");
        status = GSOUND_STATUS_ERROR;
    }
    else
    {
        status = gaa_OtaCreate(handler);

        if (status == GSOUND_STATUS_OK)
        {
            gaa_OtaSetState(GAA_OTA_STATE_READY);

            if (UpgradeIsRunningNewImage())
            {
                DEBUG_LOG("GSoundTargetOtaInit rebooted after successful upgrade");
                /* Was the upgrade caused by GAA OTA or another host? */
                gaa_ota_reboot_occurred = gaa_OtaRebootDueToUpgrade();
                if (gaa_ota_reboot_occurred)
                {
                    /* The upgrade caused by GAA OTA. Register for upgrade peer messages. */
                    appUpgradePeerClientRegister(&gaa_ota->task_data);
                }
            }
        }

        if (ota_device_config)
        {
            /* Only a non-TWS system supported at present. */
            ota_device_config->ota_device_count = 1;
        }
    }
    return status;
}

GSoundStatus GSoundTargetOtaBegin(const GSoundOtaSessionSettings *ota_settings)
{
    GSoundStatus status = GSOUND_STATUS_OK;
    if (!ota_settings)
    {
        DEBUG_LOG("GSoundTargetOtaBegin ota session settings were NULL");
        status = GSOUND_STATUS_ERROR;
    }
    else if (gaa_ota == NULL)
    {
        DEBUG_LOG("GSoundTargetOtaBegin not initialised");
        status = GSOUND_STATUS_ERROR;
    }
    else if ((gaa_ota->state != GAA_OTA_STATE_READY) &&
        (gaa_ota->state != GAA_OTA_STATE_ABORT))
    {
        DEBUG_LOG("GSoundTargetOtaBegin unexpected state %u", gaa_ota->state);
        status = GSOUND_STATUS_ERROR;
    }
    else if ((UpgradeTransportInUse()) && (gaa_ota->state == GAA_OTA_STATE_READY))
    {
        DEBUG_LOG("GSoundTargetOtaBegin upgrade already in use");
        status = GSOUND_STATUS_ERROR;
    }
    else
    {
        DEBUG_LOG("GSoundTargetOtaBegin total size %ld start offset %lu device index %lu",
            ota_settings->total_size, ota_settings->start_offset, ota_settings->device_index);
        if (ota_settings->ota_version)
        {
            DEBUG_LOG("GSoundTargetOtaBegin version %s", ota_settings->ota_version);
        }
        status = gaa_OtaCreateData(ota_settings->total_size, ota_settings->device_index);

        if (status == GSOUND_STATUS_OK)
        {
            if (gaa_ota->state == GAA_OTA_STATE_READY)
            {
                /* Call UpgradeTransportConnectRequest and expect a UPGRADE_TRANSPORT_CONNECT_CFM message back. */
                UpgradeTransportConnectRequest(&gaa_ota->task_data,
                    DATA_CFM_IS_NOT_NEEDED, DO_NOT_REQUEST_MULTIPLE_BLOCKS);
            }
            gaa_OtaSetState(GAA_OTA_STATE_BEGINNING_CONNECT);
        }
    }
    return status;
}

GSoundStatus GSoundTargetOtaData(const uint8_t *data, uint32_t length,
                                 uint32_t byte_offset,
                                 GSoundOtaDataResult *ota_status,
                                 uint32_t device_index)
{
    GSoundStatus status = GSOUND_STATUS_OK;
    UNUSED(device_index);

    /* Data consumed asynchronously and rewind offset is set when appropriate */
    memset(ota_status, 0, sizeof(ota_status));

    if (gaa_ota == NULL)
    {
        DEBUG_LOG("GSoundTargetOtaData not initialised");
        status = GSOUND_STATUS_ERROR;
    }
    else if (!gaa_ota->data)
    {
        DEBUG_LOG("GSoundTargetOtaData data array has been released");
        status = GSOUND_STATUS_ERROR;
    }
    else if ((gaa_ota->state < GAA_OTA_STATE_BEGINNING_CONNECT) || (gaa_ota->state > GAA_OTA_STATE_DATA))
    {
        DEBUG_LOG("GSoundTargetOtaData unexpected state %u", gaa_ota->state);
        status = GSOUND_STATUS_ERROR;
    }
    else if ((byte_offset + length) > gaa_ota->total_size)
    {
        DEBUG_LOG("GSoundTargetOtaData byte offset %lu + length %ld > total size %ld", byte_offset, length, gaa_ota->total_size);
        status = GSOUND_STATUS_ERROR;
    }
    else
    {
        DEBUG_LOG("GSoundTargetOtaData length %ld byte offset %lu", length, byte_offset);

        if (gaa_ota->rewind_outstanding == TRUE)
        {
            if (byte_offset == gaa_ota->resume_offset)
            {
                /*
                 * We are rewinding and have now got the correct byte_offset, so
                 * record the fact that we are no longer rewinding and continue.
                 */
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
                MessageCancelAll(&gaa_ota->task_data, GAA_OTA_INTERNAL_REWIND);
#else                 
                MessageCancelAll(&gaa_ota->task_data, GAA_OTA_INTERNAL_TIMEOUT);
#endif                
                gaa_ota->rewind_outstanding = FALSE;
                DEBUG_LOG("GSoundTargetOtaData rewound to %lu", gaa_ota->resume_offset);
            }
            else
            {
                /*
                 * We are rewinding and have not got the correct byte_offset yet, so
                 * "silently drop [this] packet" and keep doing that until the
                 * correct byte_offset eventually arrives.
                 */
            }
        }

        if (gaa_ota->rewind_outstanding == FALSE)
        {
            /* We are not still waiting for a rewind */
            size_t space = GAA_OTA_DATA_BUFFER_SIZE - (gaa_ota->end_offset - gaa_ota->start_offset);
            gaa_OtaAppendData(data, length, byte_offset, space);
            /* The gaa_OtaAppendData function may set gaa_ota->rewind_outstanding to TRUE. */
        }

        if (gaa_ota->rewind_outstanding == TRUE)
        {
            /* We are still (or again) waiting for a rewind */
            ota_status->rewind_offset = gaa_ota->resume_offset;
            DEBUG_LOG("GSoundTargetOtaData synchronous rewind to %lu", gaa_ota->resume_offset);
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
			MessageSendLater(&gaa_ota->task_data, GAA_OTA_INTERNAL_REWIND, NULL, GAA_OTA_REWIND_TIMEOUT);            
#else
            MessageSendLater(&gaa_ota->task_data, GAA_OTA_INTERNAL_TIMEOUT, NULL, GAA_OTA_TIMEOUT_DELAY);
#endif            
        }
    }
    return status;
}

GSoundStatus GSoundTargetOtaApply(uint32_t device_index)
{
    GSoundStatus status = GSOUND_STATUS_OK;
    DEBUG_LOG("GSoundTargetOtaApply");

    UNUSED(device_index);

    if (gaa_ota == NULL)
    {
        DEBUG_LOG("GSoundTargetOtaApply not initialised");
        status = GSOUND_STATUS_ERROR;
    }
    else if ((gaa_ota->state != GAA_OTA_STATE_DATA) && (gaa_ota->state != GAA_OTA_STATE_AWAITING_DATA_REQ))
    {
        DEBUG_LOG("GSoundTargetOtaApply unexpected state %u", gaa_ota->state);
        status = GSOUND_STATUS_ERROR;
    }
    else if (gaa_ota->start_offset != gaa_ota->total_size)
    {
        DEBUG_LOG("GSoundTargetOtaApply start offset %lu is not total size %lu", gaa_ota->start_offset, gaa_ota->total_size);
        status = GSOUND_STATUS_ERROR;
    }
    else
    {
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
 		gaa_OtaFreeDataBuffer();
        gaa_OtaSetState(GAA_OTA_STATE_APPLY);
        gaa_OtaSendShortMessage(UPGRADE_HOST_IS_CSR_VALID_DONE_REQ);
        MessageSendLater(&gaa_ota->task_data,
            GAA_OTA_INTERNAL_DISCONNECT,
            NULL,
            GAA_OTA_DISCONNECT_TIMEOUT);
#else    	
        if (gaa_ota->data)
        {
            free(gaa_ota->data);
            gaa_ota->data = NULL;
        }
        gaa_OtaSetState(GAA_OTA_STATE_APPLY);
        gaa_OtaSendShortMessage(UPGRADE_HOST_IS_CSR_VALID_DONE_REQ);
#endif
    }
    return status;
}

GSoundStatus GSoundTargetOtaAbort(uint32_t device_index)
{
    GSoundStatus status = GSOUND_STATUS_OK;
    DEBUG_LOG("GSoundTargetOtaAbort");

    UNUSED(device_index);

    if (gaa_ota == NULL)
    {
        DEBUG_LOG("GSoundTargetOtaAbort not initialised");
        status = GSOUND_STATUS_ERROR;
    }
    else if (gaa_ota->state == GAA_OTA_STATE_ABORT)
    {
        /* Don't try and abort if already in the process of aborting. */
        DEBUG_LOG("GOTA: Abort(): ignored as already aborting");
        status = GSOUND_STATUS_OK;
    }
    else
    {
        gaa_OtaCancelPendingRewind();
        gaa_OtaFreeDataBuffer();

        if (gaa_ota->state == GAA_OTA_STATE_READY)
        {
            DEBUG_LOG("GOTA: Abort(): ignored as not active");
        }
        else
        {
            gaa_OtaSetState(GAA_OTA_STATE_ABORT);
            gaa_OtaSendShortMessage(UPGRADE_HOST_ABORT_REQ);
        }
    }
    return status;
    
}

GSoundStatus GSoundTargetInfoGetAppVersion(uint8_t *version, uint32_t max_len)
{
    GSoundStatus status = GSOUND_STATUS_OK;
#ifdef ENABLE_TYM_PLATFORM /*for gaa support 9.9.9 */
    uint16 minor_tym, major_tym;
#endif
    /*
     * The GSound library is calling this function before it has called
     * GSoundTargetOtaInit, so cannot use the conventional mechanism of
     * connecting to the upgrade library, sending an UPGRADE_HOST_VERSION_REQ,
     * getting a UPGRADE_HOST_VERSION_CFM back in response, and disconnecting.
     * Have to resort to a "back door" call into the upgrade library.
     */
    if (!version)
    {
        DEBUG_LOG("GSoundTargetInfoGetAppVersion invalid version");
        status = GSOUND_STATUS_ERROR;
    }
    else if (!UpgradeIsInitialised())
    {
        DEBUG_LOG("GSoundTargetInfoGetAppVersion upgrade library uninitialised");
        status = GSOUND_STATUS_ERROR;
    }
    else
    {
        uint16 major, minor, config;
        bool result = FALSE;
        if (UpgradeIsRunningNewImage())
        {
            result = UpgradeGetInProgressVersion(&major, &minor, &config);
        }
        else
        {
            result = UpgradeGetVersion(&major, &minor, &config);
        }
#ifdef ENABLE_TYM_PLATFORM /*for gaa support 9.9.9 */
        major_tym = (minor/256);
        minor_tym = (minor%255);

        major = (major_tym%10);
        minor = (minor_tym/10);
        config =(minor_tym%10);
#endif

        if (result)
        {
            snprintf((char *) version, (size_t) max_len, "%u.%u.%u", major, minor, config);
            DEBUG_LOG("GSoundTargetInfoGetAppVersion %s", version);
        }
        else
        {
            DEBUG_LOG("GSoundTargetInfoGetAppVersion failed");
            status = GSOUND_STATUS_ERROR;
        }
    }
    return status;
}
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
/*remove GaaRebootDueToOtaUpgrade function */
#else
bool GaaRebootDueToOtaUpgrade(void)
{
    /*
     * This function needs to be called after GSoundTargetOtaInit, which can be
     * determined by whether the gaa_ota pointer is NULL or not.
     */
    PanicNull(gaa_ota);
    return gaa_ota_reboot_occurred;
}
#endif

void GSoundTargetOtaGetResumeInfo(GSoundOtaResumeInfo *target_resume_info, uint32_t device_index)
{
    UNUSED(device_index);

    if (UpgradePartialUpdateInterrupted())
    {
        /* TODO: How to check if all data is received but not applied (GSOUND_TARGET_OTA_RESUME_COMPLETE) */
        target_resume_info->requested_state = GSOUND_TARGET_OTA_RESUME_IN_PROGRESS;

        uint16 major, minor, config;
        bool result = UpgradeGetInProgressVersion(&major, &minor, &config);
        DEBUG_LOG("GSoundTargetOtaGetResumeInfo interrupted");
        /*
         * The upgrade library resume capability does not work with a concept
         * of a (resume) offset but with a concept of last_closed_partition. If
         * the last_closed_partition PS key is non-zero then the upgrade
         * library will "skip" any partitions with a partition number that is
         * less than the last_closed_partition. However, those partitions that
         * have already been stored in SQIF must still be provided in the data
         * transfer phase hence offset is set to zero.
         */
        target_resume_info->offset = 0;
        if (result)
        {
            snprintf((char *) target_resume_info->version,
                (size_t) GSOUND_TARGET_OTA_VERSION_MAX_LEN,
                "%u.%u.%u", major, minor, config);
            DEBUG_LOG("GSoundTargetOtaGetResumeInfo %s", target_resume_info->version);
        }
        else
        {
            DEBUG_LOG("GSoundTargetOtaGetResumeInfo version not available");
        }
    }
    else
    {
        target_resume_info->requested_state = GSOUND_TARGET_OTA_RESUME_NONE;
        DEBUG_LOG("GSoundTargetOtaGetResumeInfo not interrupted");
    }
}

void GSoundTargetOtaGetStatus(GSoundOtaStatus *target_ota_status, uint32_t device_index)
{
    UNUSED(device_index);
    bool in_use = UpgradeTransportInUse();
    if (in_use && gaa_ota && (gaa_ota->state != GAA_OTA_STATE_READY))
    {
        /*
         * The device is currently receiving an OTA, but it is not from a source
         * other than from this GAA OTA entity, so change the result to FALSE.
         */
        in_use = FALSE;
    }
    DEBUG_LOG("GSoundTargetOtaGetStatus %d", in_use);
    target_ota_status->is_receiving_other = in_use;
}
