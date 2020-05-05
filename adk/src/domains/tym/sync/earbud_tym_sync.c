/*!
\copyright  Copyright (c) 2017 - 2019 TYMPHANY Technologies International, Ltd.
            All Rights Reserved.
            TYMPHANY Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       tym_sync.c
\brief      Support for the tym sync
*/
/* ------------------------ Include ------------------------ */
#include <panic.h>
#include <pio.h>
#include <stdlib.h>
#include <peer_signalling.h>
#include <ps.h>
#include "earbud_tym_sync.h"
#include "earbud_tym_sync_marshal_defs.h"
#include "earbud_tym_sync_private.h"
#include "adk_log.h"
#include "earbud_tym_cc_communication.h"
#include "logical_input_switch.h"
#include "phy_state.h"
#include "1_button.h"
#include "tym_anc.h"
#include "earbud_tym_psid.h"

/* ------------------------ Defines ------------------------ */

#define xprint(x)            DEBUG_LOG(x)
#define xprintf(x, ...)      DEBUG_LOG(x,  __VA_ARGS__)

/* ------------------------ Types -------------------------- */

/* --------------- Local Function Prototypes --------------- */

static void tymSync_HandleMarshalledMsgChannelTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* cfm);
/*! \brief Handle incoming marshalled messages from peer tym sync component. */
static void tymSync_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind);
static void tymSync_HandleMessage(Task task, MessageId id, Message message);
static void handleSync_Command(tym_sync_data_t* tym_data);
static void handleSync_AppConfig(tym_sync_app_configuration_t* tym_app_configuration);
/* --------------- Static Global Variables ----------------- */
/*! \brief Key Sync task data. */
tym_sync_task_data_t tym_sync ={0};
/* ------------------ Local Function ----------------------- */
/*! \brief Handle confirmation of transmission of a marshalled message. */
static void tymSync_HandleMarshalledMsgChannelTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* cfm)
{
    xprintf("tymSync_HandleMarshalledMsgChannelTxCfm channel %u status %u", cfm->channel, cfm->status);
}

/*! \brief Handle incoming marshalled messages from peer tym sync component. */
static void tymSync_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{
    xprint("tymSync_HandleMarshalledMsgChannelRxInd channel");
    switch (ind->type)
    {
        case MARSHAL_TYPE(tym_sync_data_t):
            {
                xprint("tym Sync_HandleMarshalledMsgChannelRxInd tym sync data to add");
                tym_sync_data_t* req = (tym_sync_data_t*)ind->msg;
                handleSync_Command(req);
            }
            break;
        case MARSHAL_TYPE(tym_sync_app_configuration_t):
            {
                tym_sync_app_configuration_t* config = (tym_sync_app_configuration_t*)ind->msg;
                handleSync_AppConfig(config);
            }
            break;    
        default:
            break;       
    }
    free(ind->msg);
            
}

/*! Key Sync Message Handler. */
static void tymSync_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    
    switch (id)
    {
            /* marshalled messaging */
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            tymSync_HandleMarshalledMsgChannelRxInd((PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T*)message);
            break;
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            tymSync_HandleMarshalledMsgChannelTxCfm((PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T*)message);
            break;
        default:
            break;
    }
}

/*! no case trigger pairing process*/
static void noCasePairProcess(uint8 context)
{
    xprintf("noCasePairProcess context 0x%x",context);
    if(context & 0x01) //5s trigger
    {
        if(context & TYM_LEFT_EARBUD)
        {
            tym_sync.leftpairact = TRUE;
        }
        else
        {
            tym_sync.rightpairact = TRUE; 
        }
        DEBUG_LOGF("noCasePairProcess left %d,right %d",tym_sync.leftpairact,tym_sync.rightpairact);
        if( (tym_sync.leftpairact == TRUE) && (tym_sync.rightpairact == TRUE) )
        {
            DEBUG_LOGF("noCasePairProcess send pairing");
            MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_HANDSET_PAIRING, NULL); 
        }    
                    
    }
    else //5s trigger release
    {
        if(context & TYM_LEFT_EARBUD)
        {
            tym_sync.leftpairact = FALSE;
        }
        else
        {
           tym_sync.rightpairact = FALSE;  
        }   
    }        
}


/*! modify ambient level */
static void moditifyAmbientLevel(uint8 context)
{
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    tymAnc->ambientLevel = context;    
}

/*! modify speech level */
static void moditifySpeechLevel(uint8 context)
{
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    tymAnc->speechLevel = context;      
}

/*! handle earbud sync app configuration. */
static void handleSync_AppConfig(tym_sync_app_configuration_t* tym_app_configuration)
{
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    /*To do*/
    if(memcmp(app_set, tym_app_configuration, sizeof(tym_sync_app_configuration_t)) != 0)
    {
        DEBUG_LOG("Ui custom %d %d %d %d",tym_app_configuration->custom_ui[0],tym_app_configuration->custom_ui[1],tym_app_configuration->custom_ui[2],tym_app_configuration->custom_ui[3]);
        memcpy(app_set, tym_app_configuration, sizeof(tym_sync_app_configuration_t));
        PsStore(PSID_APPCONFIG, tym_app_configuration, PS_SIZE_ADJ(sizeof(tym_sync_app_configuration_t)));
    }
}

/*! handle earbud sync data. */
static void handleSync_Command(tym_sync_data_t* tym_data)
{
    uint8 cmd = tym_data->cmd;
    switch(cmd)
    {
        case btStatusCmd:
            xprintf("tym bt status %d",tym_data->data);
            reportBtStatus(tym_data->data);
            break;
        case noCasePairCmd:
            noCasePairProcess(tym_data->data);
            break;
        case ambientLevelCmd:
            moditifyAmbientLevel(tym_data->data);
            break;
        case speechLevelCmd:
            moditifySpeechLevel(tym_data->data);
            break;        
        case sleepStandbyModeCmd:
            appPhyChangeSleepStandbyMode(tym_data->data);
            break;    
        default:
            xprint("can't find command");        
    }
}
/* ----------------- Global Function ----------------------- */
bool TymSync_Init(Task init_task)
{
    tym_sync_task_data_t *ts = tymSync_GetTaskData();

    UNUSED(init_task);

    xprint("TymSync_Init");

    /* Initialise component task data */
    memset(ts, 0, sizeof(*ts));
    ts->task.handler = tymSync_HandleMessage;

    /* Register with peer signalling to use the key sync msg channel */
    appPeerSigMarshalledMsgChannelTaskRegister(tymSync_GetTask(),
                                               PEER_SIG_MSG_CHANNEL_TYM_SYNC,
                                               tym_sync_marshal_type_descriptors,
                                               NUMBER_OF_MARSHAL_OBJECT_TYPES);
    return TRUE;
}

void tymSyncdata(uint8 command,uint8 data)
{
    tym_sync_task_data_t *ts = tymSync_GetTaskData();    
    tym_sync_data_t* tym_data;
    
    if(ts->task.handler != NULL)
    {    
        tym_data = PanicUnlessMalloc(sizeof(tym_sync_data_t));
        /* send to counterpart on other earbud */
        tym_data->cmd = command;
        tym_data->data = data;
        appPeerSigMarshalledMsgChannelTx(tymSync_GetTask(),
                                     PEER_SIG_MSG_CHANNEL_TYM_SYNC,
                                      tym_data, MARSHAL_TYPE_tym_sync_data_t); 
        handleSync_Command(tym_data);
    }
}


void tymSyncAppConfiguration(tym_sync_app_configuration_t *config)
{
    tym_sync_task_data_t *ts = tymSync_GetTaskData();    
    tym_sync_app_configuration_t* tym_config;  
    if(ts->task.handler != NULL)
    {    
        tym_config = PanicUnlessMalloc(sizeof(tym_sync_app_configuration_t));
        /* send to counterpart on other earbud */
        memcpy(tym_config, config, sizeof(tym_sync_app_configuration_t));
        appPeerSigMarshalledMsgChannelTx(tymSync_GetTask(),
                                     PEER_SIG_MSG_CHANNEL_TYM_SYNC,
                                      tym_config, MARSHAL_TYPE_tym_sync_app_configuration_t);
        //handleSync_AppConfig(tym_config);
    } 
}

