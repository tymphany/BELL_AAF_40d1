    /*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation for Gaa TWS. Refer to gsound_target_tws.h for API information.
*/

#include "gsound_target_tws.h"
#include "gsound_target_bt.h"
#include "gsound_service.h"
#include "tws_topology_role_change_client_if.h"
#include "gatt_gaa_comm_server.h"
#include "gaa_tws.h"
#include "gaa_private.h"
#include <logging.h>
#include <panic.h>
#include <voice_ui.h>

/*! GSound TWS Interface */
static const GSoundTwsInterface *tws_handlers;

static bool role_state_initialised = FALSE;
static bool pending_role_change_expected = FALSE;
static bool rfcomm_disconnection_confirmation_required = FALSE;
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
static tws_topology_role preRoleSwitchRole = tws_topology_role_none;
#endif
static Task serverTask = NULL; 

static int32_t min_reconnection_delay = 5000;
static int32_t disconnection_timeout = 200;
static void gaa_tws_InternalMessageHandler(Task task, MessageId id, Message message);

static TaskData gaa_tws_task_data = {gaa_tws_InternalMessageHandler};

TaskData *Gaa_GetTwsTask(void)
{
    return &gaa_tws_task_data;
}

static void gaa_tws_HandleTwsTopologyInitialRole(tws_topology_role role);
static void gaa_tws_HandleTwsTopologyRoleChange(tws_topology_role role);
static void gaa_tws_UpdateTwsTopology(bool role_change_accepted);

/* role change interface implementation prototypes */
static void gaa_tws_Initialise(Task server, int32_t reconnect_delay);
static void gaa_tws_RoleChangeIndication(tws_topology_role role);
static void gaa_tws_ProposeRoleChange(void);
static void gaa_tws_ForceRoleChange(void);
static void gaa_tws_CancelRoleChange(void);
static void gaa_tws_PrepareRoleChange(void);

void GSoundTargetTwsInit(const GSoundTwsInterface *handler)
{
    DEBUG_LOG("GSoundTargetTwsInit handler =%p",handler);
    tws_handlers = handler;
    role_state_initialised = FALSE;
    rfcomm_disconnection_confirmation_required = FALSE;
    pending_role_change_expected = FALSE;
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
    preRoleSwitchRole = tws_topology_role_none;
#endif
}

static void gaa_tws_Initialise(Task server, int32_t reconnect_delay)
{
    DEBUG_LOG("gaa_tws_Initialise server = 0x%p, reconnect delay = %d",server,reconnect_delay);
    min_reconnection_delay = reconnect_delay;
    serverTask = server;
}

static bool gaa_is_ble_disconnected(void)
{
    return (GattGaaCommServerGetCid() == INVALID_CID);
}
static void gaa_tws_ProposeRoleChange(void)
{
    DEBUG_LOG("gaa_tws_ProposeRoleChange");
    PanicFalse(tws_handlers && role_state_initialised);
    tws_handlers->gsound_tws_role_change_request();
}

static void gaa_tws_ForceRoleChange(void)
{
    DEBUG_LOG("gaa_tws_ForceRoleChange");
    PanicFalse(tws_handlers && role_state_initialised);
    rfcomm_disconnection_confirmation_required = gaa_is_ble_disconnected();
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
    pending_role_change_expected = TRUE;
#endif
    tws_handlers->gsound_tws_role_change_force(min_reconnection_delay);
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
	if(rfcomm_disconnection_confirmation_required)
    {
        DEBUG_LOG("gaa_tws_ForceRoleChange, starting disconnection timer");
        MessageSendLater(Gaa_GetTwsTask(), GAA_INTERNAL_TWS_DISCONNECTION_TIMEOUT, NULL, disconnection_timeout);
    }
#endif
}

static void gaa_tws_CancelRoleChange(void)
{
    DEBUG_LOG("gaa_tws_CancelRoleChange");
    PanicFalse(tws_handlers && role_state_initialised);
    if(rfcomm_disconnection_confirmation_required)
    {
        MessageCancelAll(Gaa_GetTwsTask(), GAA_INTERNAL_TWS_DISCONNECTION_TIMEOUT);
    }
    tws_handlers->gsound_tws_role_change_cancel();
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
    pending_role_change_expected = FALSE;
#else    
    pending_role_change_expected = TRUE;
#endif    
}

void GSoundTargetTwsRoleChangeResponse(bool accepted)
{
    if(accepted)
    {
        DEBUG_LOG("gaa_tws GSoundTargetTwsRoleChangeResponse: ACCEPTED");
        gaa_tws_UpdateTwsTopology(TRUE);
    }
    else
    {
        DEBUG_LOG("gaa_tws GSoundTargetTwsRoleChangeResponse: ROLE CHANGE NOT ACCEPTED");
        gaa_tws_UpdateTwsTopology(FALSE);
    }
}

static void gaa_tws_PrepareRoleChange(void)
{
    DEBUG_LOG("gaa_tws_PrepareRoleChange");
    PanicFalse(tws_handlers && role_state_initialised);
    rfcomm_disconnection_confirmation_required = gaa_is_ble_disconnected();
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
    pending_role_change_expected = TRUE;
#endif
    tws_handlers->gsound_tws_role_change_perform(min_reconnection_delay);
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
    if(rfcomm_disconnection_confirmation_required)
    {
        DEBUG_LOG("gaa_tws_PrepareRoleChange, starting disconnection timer");  
        MessageSendLater(Gaa_GetTwsTask(), GAA_INTERNAL_TWS_DISCONNECTION_TIMEOUT, NULL, disconnection_timeout);
    }
#endif    
}

void GSoundTargetTwsRoleChangeGSoundDone(void)
{
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
	DEBUG_LOG("gaa_tws: GSoundTargetTwsRoleChangeGSoundDone");
   	MessageSend(PanicNull(serverTask), TWS_ROLE_CHANGE_PREPARATION_CFM, NULL); 
#else	
    if(pending_role_change_expected == FALSE)
    {  
        DEBUG_LOG("gaa_tws: GSoundTargetTwsRoleChangeGSoundDone");
        pending_role_change_expected = TRUE;
        if(rfcomm_disconnection_confirmation_required)
        {
            MessageSendLater(Gaa_GetTwsTask(), GAA_INTERNAL_TWS_DISCONNECTION_TIMEOUT, NULL, disconnection_timeout);
        }
        else
        {
            MessageSend(PanicNull(serverTask), TWS_ROLE_CHANGE_PREPARATION_CFM, NULL);
        }
    }
    else
       DEBUG_LOG("gaa_tws: Unexpected GSoundTargetTwsRoleChangeGSoundDone"); 
#endif       
}

static void gaa_tws_DisconnectionConfirmed(void)
{
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
    DEBUG_LOG("gaa_tws_DisconnectionConfirmed, cancel disconnection timer");
#else	
    DEBUG_LOG("gaa_tws_DisconnectionConfirmed");
#endif    
    MessageCancelAll(Gaa_GetTwsTask(), GAA_INTERNAL_TWS_DISCONNECTION_TIMEOUT);
    MessageSend(PanicNull(serverTask), TWS_ROLE_CHANGE_PREPARATION_CFM, NULL);
}

static void gaa_tws_UpdateTwsTopology(bool role_change_accepted)
{
    MAKE_TWS_ROLE_CHANGE_ACCEPTANCE_MESSAGE(TWS_ROLE_CHANGE_ACCEPTANCE_CFM);
    message->role_change_accepted = role_change_accepted;

    MessageSend(PanicNull(serverTask), TWS_ROLE_CHANGE_ACCEPTANCE_CFM, message);
}

static void gaa_tws_RoleChangeIndication(tws_topology_role role)
{
    /* Process Role Change Indication, depending on initialisation flag state */
    if(role_state_initialised)
        gaa_tws_HandleTwsTopologyRoleChange(role);
    else
        gaa_tws_HandleTwsTopologyInitialRole(role);
}

static void gaa_tws_HandleTwsTopologyInitialRole(tws_topology_role role)
{
    bool initialised = TRUE;
    switch(role)
    {
        case tws_topology_role_none:
        case tws_topology_role_dfu:
        case tws_topology_role_shutdown:
            DEBUG_LOG("gaa_tws_HandleTwsTopologyInitialRole iqnored initial role (%u)",role);
            initialised = FALSE;
            break;   
   
        case tws_topology_role_primary:
            PanicFalse(tws_handlers);
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
            preRoleSwitchRole = tws_topology_role_primary;
#endif
            tws_handlers->gsound_tws_role_change_init_role(TRUE);
            break;
            
        case tws_topology_role_secondary:
            PanicFalse(tws_handlers);
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
            preRoleSwitchRole = tws_topology_role_secondary;
#endif
            tws_handlers->gsound_tws_role_change_init_role(FALSE);
            break;
    }
    role_state_initialised = initialised;
}
    
static void gaa_tws_HandleTwsTopologyRoleChange(tws_topology_role role)
{
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
    DEBUG_LOG("gaa_tws_HandleTwsTopologyRoleChange, new role is %d", role);
    DEBUG_LOG("gaa_tws_HandleTwsTopologyRoleChange pending_role_change_expected is %d",pending_role_change_expected);
    DEBUG_LOG("gaa_tws_HandleTwsTopologyRoleChange, preRoleSwitchRole is %d", preRoleSwitchRole);	
#endif
    PanicFalse(tws_handlers && role_state_initialised);
    if (role == tws_topology_role_secondary)
    {  
        if(pending_role_change_expected)
        {
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
			if (preRoleSwitchRole == tws_topology_role_primary )
          	{
              	DEBUG_LOG("gaa_tws_HandleTwsTopologyRoleChange (primary->secondary)");
              	tws_handlers->gsound_tws_role_change_target_done(FALSE);
              	pending_role_change_expected = FALSE;
            	preRoleSwitchRole = tws_topology_role_secondary;
          	}
          	else if (preRoleSwitchRole == tws_topology_role_secondary)
          	{
              	DEBUG_LOG("gaa_tws_HandleTwsTopologyRoleChange (secondary->secondary)");
              	pending_role_change_expected = FALSE;
          	}
#else        	
            DEBUG_LOG("gaa_tws_HandleTwsTopologyRoleChange (primary->secondary:expected)");
            tws_handlers->gsound_tws_role_change_target_done(FALSE);
            pending_role_change_expected = FALSE;
#endif
        }
        else 
        {
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
			if (preRoleSwitchRole == tws_topology_role_primary )
            {
                DEBUG_LOG("gaa_tws_HandleTwsTopologyRoleChange (primary->secondary:fatal)");
                tws_handlers->gsound_tws_role_change_fatal(FALSE);
                preRoleSwitchRole = tws_topology_role_secondary;
            }
#else        	
            DEBUG_LOG("gaa_tws_HandleTwsTopologyRoleChange (primary->secondary:fatal)");
            tws_handlers->gsound_tws_role_change_fatal(FALSE);
#endif
        }
    }
    else if (role == tws_topology_role_primary)
    {
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch*/
        if(pending_role_change_expected)
        {
            if (preRoleSwitchRole == tws_topology_role_primary)
            {
                DEBUG_LOG("gaa_tws_HandleTwsTopologyRoleChange (primary->primary)");
                pending_role_change_expected = FALSE;
                tws_handlers->gsound_tws_role_change_cancel();
            }
        }
        else 
        {
            if (preRoleSwitchRole == tws_topology_role_secondary )
            {
                DEBUG_LOG("gaa_tws_HandleTwsTopologyRoleChange (secondary->primary)");
                preRoleSwitchRole = tws_topology_role_primary;
                tws_handlers->gsound_tws_role_change_target_done(TRUE);
            }
        }
#else    	
        DEBUG_LOG("gaa_tws_HandleTwsTopologyRoleChange (secondary->primary:controlled/forced/fatal)");
        tws_handlers->gsound_tws_role_change_target_done(TRUE);
#endif
    }
    else
    {
        DEBUG_LOG("gaa_tws_HandleTwsTopologyRoleChange iqnored role change (%u)",role);
    }
}

static void gaa_tws_PerformTimeoutDisconnect(void)
{
    if(GSOUND_STATUS_OK != GSoundTargetBtChannelClose(GSOUND_CHANNEL_CONTROL))
        DEBUG_LOG("gaa_tws_PerformTimeoutDisconnect: Failed to disconnect RFComm GSOUND_CHANNEL_CONTROL ");    
     
    if(GSOUND_STATUS_OK != GSoundTargetBtChannelClose(GSOUND_CHANNEL_AUDIO))
        DEBUG_LOG("gaa_tws_PerformTimeoutDisconnect: Failed to disconnect RFComm GSOUND_CHANNEL_AUDIO ");    
}

static void gaa_tws_InternalMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch(id)
    {
        case GAA_INTERNAL_TWS_DISCONNECTION_IND:
            if(pending_role_change_expected)
                gaa_tws_DisconnectionConfirmed();
            else
                DEBUG_LOG("Non expected GAA_INTERNAL_TWS_DISCONNECTION_IND");
            break;    
        
        case GAA_INTERNAL_TWS_DISCONNECTION_TIMEOUT:
            DEBUG_LOG("GAA_INTERNAL_TWS_DISCONNECTION_TIMEOUT");
            gaa_tws_PerformTimeoutDisconnect();
            break;

        default:
            DEBUG_LOG("gaa_tws_InternalMessageHandler unhandled 0x%04X", id);
            break;
    }
}


#ifdef HOSTED_TEST_ENVIRONMENT  
void GaaTWSReset(void)
{
    tws_handlers = NULL;
    
    role_state_initialised = FALSE;
    pending_role_change_expected = FALSE;    
}
#endif

TWS_ROLE_CHANGE_CLIENT_REGISTRATION_MAKE(GAA_TWS, gaa_tws_Initialise, gaa_tws_RoleChangeIndication,gaa_tws_ProposeRoleChange,
                                         gaa_tws_ForceRoleChange,gaa_tws_PrepareRoleChange,gaa_tws_CancelRoleChange);

