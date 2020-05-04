/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
                        All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation for GAA On-Head Detection. Refer to gsound_target_ohd.h for API information.
*/

#include "gsound_target.h"
#include "gsound_target_ohd.h"

#include "multidevice.h"
#include "phy_state.h"
#include "state_proxy.h"

#include <logging.h>

static GSoundOhdState gaa_OhdGetStateFromMultiPartTable(bool self_in_ear, bool peer_in_ear, multidevice_side_t self_side);
static GSoundOhdState gaa_OhdGetStateFromSinglePartTable(bool in_ear);
static GSoundOhdState gaa_OhdUpdateStatus(void);
static void gaa_OhdHandleStateProxyEvent(const STATE_PROXY_EVENT_T *ind);
static void gaa_OhdMessageHandler(Task task, MessageId id, Message message);

static const GSoundOhdInterface *ohd_handlers;
static TaskData gaa_ohd_task = {gaa_OhdMessageHandler};

typedef struct
{
    bool self_in_ear;
    bool peer_in_ear;
    multidevice_side_t self_side;
    GSoundOhdState state;
} multi_part_ohd_states_t;

typedef struct
{
    bool in_ear;
    GSoundOhdState state;
} single_part_ohd_states_t;

static const multi_part_ohd_states_t multi_part_ohd_state_table[] =
{
    {.self_in_ear = FALSE, .peer_in_ear = FALSE, .self_side = multidevice_side_left,  OHD_NONE_DETECTED},
    {.self_in_ear = FALSE, .peer_in_ear = FALSE, .self_side = multidevice_side_right, OHD_NONE_DETECTED},

    {.self_in_ear = TRUE,  .peer_in_ear = TRUE,  .self_side = multidevice_side_left,  OHD_BOTH_DETECTED},
    {.self_in_ear = TRUE,  .peer_in_ear = TRUE,  .self_side = multidevice_side_right, OHD_BOTH_DETECTED},

    {.self_in_ear = TRUE,  .peer_in_ear = FALSE, .self_side = multidevice_side_left,  OHD_LEFT_DETECTED},
    {.self_in_ear = TRUE,  .peer_in_ear = FALSE, .self_side = multidevice_side_right, OHD_RIGHT_DETECTED},

    {.self_in_ear = FALSE, .peer_in_ear = TRUE,  .self_side = multidevice_side_left,  OHD_RIGHT_DETECTED},
    {.self_in_ear = FALSE, .peer_in_ear = TRUE,  .self_side = multidevice_side_right, OHD_LEFT_DETECTED}
};

static const single_part_ohd_states_t single_part_ohd_state_table[] =
{
    {.in_ear = FALSE, OHD_NONE_DETECTED},
    {.in_ear = TRUE,  OHD_SINGULAR_DETECTED}
};

static GSoundOhdState gaa_OhdGetStateFromMultiPartTable(bool self_in_ear, bool peer_in_ear, multidevice_side_t self_side)
{
    GSoundOhdState status = OHD_UNKNOWN;

    for(unsigned i=0; i<ARRAY_DIM(multi_part_ohd_state_table); i++)
    {
        if(multi_part_ohd_state_table[i].self_in_ear == self_in_ear &&
           multi_part_ohd_state_table[i].peer_in_ear == peer_in_ear &&
           multi_part_ohd_state_table[i].self_side == self_side)
        {
            status = multi_part_ohd_state_table[i].state;
            break;
        }
    }

    return status;
}

static GSoundOhdState gaa_OhdGetStateFromSinglePartTable(bool in_ear)
{
    GSoundOhdState status = OHD_UNKNOWN;

    for(unsigned i=0; i<ARRAY_DIM(single_part_ohd_state_table); i++)
    {
        if(single_part_ohd_state_table[i].in_ear == in_ear)
        {
            status = single_part_ohd_state_table[i].state;
            break;
        }
    }

    return status;
}

static GSoundOhdState gaa_OhdUpdateStatus(void)
{
    GSoundOhdState status = OHD_UNKNOWN;

    if(appPhyStateIsInEarDetectionSupported())
    {
        multidevice_side_t side = Multidevice_GetSide();

        if(side == multidevice_side_both)
        {
            status = gaa_OhdGetStateFromSinglePartTable(StateProxy_IsInEar());
        }
        else
        {
            status = gaa_OhdGetStateFromMultiPartTable(StateProxy_IsInEar(), StateProxy_IsPeerInEar(), side);
        }
    }
    else
    {
        status = OHD_UNSUPPORTED;
    }

    PanicFalse(ohd_handlers);
    DEBUG_LOG("gaa_OhdUpdateStatus status %u", status);
    ohd_handlers->gsound_ohd_status(status);

    return status;
}

static void gaa_OhdHandleStateProxyEvent(const STATE_PROXY_EVENT_T* sp_event)
{
    DEBUG_LOG("gaa_OhdHandleStateProxyEvent source %u type %u", sp_event->source, sp_event->type);
    switch(sp_event->type)
    {
        case state_proxy_event_type_phystate:
            /* Update for both local and remote state proxy events */
            gaa_OhdUpdateStatus();
            break;

        default:
            break;
    }
}

static void gaa_OhdMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
        case STATE_PROXY_EVENT:
            gaa_OhdHandleStateProxyEvent((const STATE_PROXY_EVENT_T *)message);
            break;

        default:
            break;
    }
}

GSoundStatus GSoundTargetGetOhdState(GSoundOhdState *ohd_status_out)
{
    DEBUG_LOG("GSoundTargetGetOhdState");
    *ohd_status_out = gaa_OhdUpdateStatus();
    return GSOUND_STATUS_OK;
}

GSoundStatus GSoundTargetOhdInit(const GSoundOhdInterface *handlers)
{
    DEBUG_LOG("GSoundTargetOhdInit");
    ohd_handlers = handlers;
    StateProxy_EventRegisterClient(&gaa_ohd_task, state_proxy_event_type_phystate);
    return GSOUND_STATUS_OK;
}
