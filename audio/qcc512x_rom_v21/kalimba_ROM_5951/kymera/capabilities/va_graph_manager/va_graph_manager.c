/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  va_graph_manager.c
 * \ingroup  capabilities
 *
 *  Manage the VA subgraph as delegated by the application client.
 *  This operator will be assigned to manage the VA operators (VAD and QVA).
 *  VA Graph Manager knows of the characteristics of the VA operators, the
 *  VA chain put in place and the whole VA use-case.
 *  The VA Graph Manager will configure the VA operators as well as the system
 *  for the VA chain to implement our solution for the VA use-case.
 *
 *  Note: this capability does not process any data. It cannot be connected to
 *  any other operator or endpoint. It only responds to events by sending
 *  requests and configurations to the VA operators and the system.
 */

#include "va_graph_manager.h"

#include "capabilities.h"
#include "fault/fault.h"

#include "opmgr/opmgr_op_client_interface.h"
#include "aov_interface/aov_interface.h"

/**
 * Our current implementation doesn't need operators to be started and stopped,
 * but the va graph manager, as an operator client, has the power to do so.
 * Code under the following flag is an example of handling the auxiliary
 * behaviour for sending start/stop commands to operators.
 *
 * #define GM_SENDS_COMMANDS
 */

#ifdef GM_SENDS_COMMANDS
typedef enum OPERATOR_COMMAND
{
    COMMAND_START,
    COMMAND_STOP,
    COMMAND_RESET,
    COMMAND_NONE
}OPERATOR_COMMAND;
#endif

typedef struct VAGM_OP_DATA
{
    OPERATOR_ID vad_op_id;
    OPERATOR_ID qva_op_id;
    OPERATOR_ID cvc_op_id;
#ifdef GM_SENDS_COMMANDS
    OPERATOR_COMMAND issued_cmd;
    OPERATOR_ID cmd_operator;
#endif
    OPMSG_VA_GM_LOAD graph_load;
    bool wait_for_vad;
    bool wait_for_qva;
    bool wait_for_aov;
    bool voice_activity;
    bool lp_active;
}VAGM_OP_DATA;

/****************************************************************************
Private Function Definitions
*/
static void vagm_init(OPERATOR_DATA *op_data);
static void vagm_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);
static bool vagm_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
static bool vagm_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
static bool vagm_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
static bool vagm_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
static bool vagm_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

static bool vagm_message_response_handler(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
#ifdef GM_SENDS_COMMANDS
static bool vagm_command_response_handler(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
#endif
static bool vagm_delegated_ops(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
static bool vagm_trigger(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
static bool vagm_negative_trigger(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
static bool vagm_lp_notification(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
static bool vagm_aov_response(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
static bool vagm_set_graph_load(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

static bool manage_clock(OPERATOR_DATA *op_data, bool custom_clock);

/****************************************************************************
Private Constant Declarations
*/

/* We need at least VAD and SVA to be delegated. */
#define VAGM_MIN_DELEGATED_OPERATORS 2
/* We currently support only one additional optional
 * delegated operator (CVC_Send).*/
#define VAGM_MAX_DELEGATED_OPERATORS 3

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define VA_GRAPH_MANAGER_ID CAP_ID_DOWNLOAD_VA_GRAPH_MANAGER
#else
#define VA_GRAPH_MANAGER_ID CAP_ID_VA_GRAPH_MANAGER
#endif

/** The stub capability function handler table */
const handler_lookup_struct vagm_handler_table =
{
    base_op_create,       /* OPCMD_CREATE */
    base_op_destroy,      /* OPCMD_DESTROY */
    base_op_start,        /* OPCMD_START */
    base_op_stop,         /* OPCMD_STOP */
    base_op_reset,        /* OPCMD_RESET */
    vagm_connect,          /* OPCMD_CONNECT */
    vagm_disconnect,       /* OPCMD_DISCONNECT */
    vagm_buffer_details,   /* OPCMD_BUFFER_DETAILS */
    vagm_get_data_format,  /* OPCMD_DATA_FORMAT */
    vagm_get_sched_info    /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table - this is the set of operator
 * messages that the capability understands and will attempt to service. */
const opmsg_handler_lookup_table_entry vagm_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {OPMSG_OP_CLIENT_REPLY_ID_DELEGATED_OPERATORS, vagm_delegated_ops},

    {OPMSG_OP_CLIENT_REPLY_ID_MESSAGE_RESPONSE, vagm_message_response_handler},
#ifdef GM_SENDS_COMMANDS
    {OPMSG_OP_CLIENT_REPLY_ID_COMMAND_RESPONSE, vagm_command_response_handler},
#endif
    {OPMSG_OP_CLIENT_REPLY_ID_VA_TRIGGER, vagm_trigger},
    {OPMSG_OP_CLIENT_REPLY_ID_VA_NEGATIVE_TRIGGER, vagm_negative_trigger},
    {OPMSG_OP_CLIENT_REPLY_ID_AOV_LP_NOTIFICATION, vagm_lp_notification},
    {OPMSG_OP_CLIENT_REPLY_ID_AOV_RESPONSE, vagm_aov_response},
    {OPMSG_VA_GM_ID_SET_GRAPH_LOAD, vagm_set_graph_load},

    {0, NULL}
};


/* Capability data - This is the definition of the capability that Opmgr uses to
 * create the capability from. */
const CAPABILITY_DATA va_graph_manager_cap_data =
{
    VA_GRAPH_MANAGER_ID,       /* Capability ID */
    0, 0,                      /* Version information - hi and lo parts */
    0, 0,                      /* Max number of sinks/inputs and sources/outputs */
    &vagm_handler_table,       /* Pointer to message handler function table */
    vagm_opmsg_handler_table,  /* Pointer to operator message handler function table */
    vagm_process_data,         /* Pointer to data processing function */
    0,                         /* Reserved */
    sizeof(VAGM_OP_DATA)       /* Size of capability-specific per-instance data */
};


#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_VA_GRAPH_MANAGER, VAGM_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_VA_GRAPH_MANAGER, VAGM_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

static inline VAGM_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (VAGM_OP_DATA *) base_op_get_instance_data(op_data);
}

/**
 * \brief Check if the VA Graph Manager has send a request and is waiting for
 *        a response.
 * \param ext_data  Capability specific data
 *
 * \return TRUE If VA Graph Manager is waiting for a response
 */
static inline bool waiting_for_any_response(VAGM_OP_DATA *ext_data)
{
    return (ext_data->wait_for_vad ||
            ext_data->wait_for_qva ||
            ext_data->wait_for_aov);
}

/**
 * \brief Initialize the capability specific data.
 *
 * \param op_data Pointer to the operator instance data.
 */
static void vagm_init(OPERATOR_DATA *op_data)
{
    VAGM_OP_DATA * ext_data = get_instance_data(op_data);
    /* (By default 32MHz is supported) */
    ext_data->graph_load = OPMSG_VA_GM_LOAD_LOW;
    ext_data->wait_for_qva = FALSE;
    ext_data->wait_for_vad = FALSE;
    ext_data->wait_for_aov = FALSE;
    ext_data->voice_activity = FALSE;
    ext_data->lp_active = FALSE;
#ifdef GM_SENDS_COMMANDS
    ext_data->issued_cmd = COMMAND_NONE;
#endif
}


/**
 * \brief Data processing function: No data to process.
 *
 * \param op_data Pointer to the operator instance data.
 * \param touched Terminals to kick. Not used.
 */
static void vagm_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    /*
     * Do nothing!
     */
}

/**
 * \brief Function to connect to a buffer. Not supported.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the connect request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
static bool vagm_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
}

/**
 * \brief Function to disconnect to a buffer. Not supported.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the disconnect request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
static bool vagm_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
}

/**
 * \brief Function to get buffer details. Not supported.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
static bool vagm_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    bool result = base_op_buffer_details(op_data, message_data, response_id, response_data);

    return result;
}

/**
 * \brief Function to get scheduling info details. Not supported.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
static bool vagm_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    OP_SCHED_INFO_RSP* resp;

    resp = base_op_get_sched_info_ex(op_data, message_data, response_id);
    if (resp == NULL)
    {
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
    }
    *response_data = resp;

    resp->block_size = 1;

    return TRUE;
}

/**
 * \brief Function to get data format. Not supported.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
static bool vagm_get_data_format(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
}

/**
 * \brief Configure VAD into the specified mode.
 *        VAD can work in full processing mode or in passthrough mode.
 *
 * \param op_data Pointer to the operator instance data.
 * \param mode Operation mode to set VAD.
 */
static void configure_vad(OPERATOR_DATA *op_data, OPMSG_VAD_MODE mode)
{
    VAGM_OP_DATA *ext_data = get_instance_data(op_data);
    unsigned msg[OPMSG_VAD_MODE_CHANGE_WORD_SIZE];

    L3_DBG_MSG("VA_GM: Configuring VAD");

    OP_CLIENT_MSG_FIELD_SET(msg, OPMSG_VAD_MODE_CHANGE, MESSAGE_ID, OPMSG_VAD_ID_MODE_CHANGE);
    OP_CLIENT_MSG_FIELD_SET(msg, OPMSG_VAD_MODE_CHANGE, WORKING_MODE, mode);

    opmgr_op_client_send_message(op_data, ext_data->vad_op_id,
                                 OPMSG_VAD_MODE_CHANGE_WORD_SIZE,
                                 (unsigned *) &msg);
}

/**
 * \brief Configure QVA into the specified mode.
 *        QVA can work in full processing mode or in passthrough mode.
 *
 * \param op_data Pointer to the operator instance data.
 * \param mode Operation mode to set QVA.
 */
static void configure_qva(OPERATOR_DATA *op_data, OPMSG_QVA_MODE mode)
{
    VAGM_OP_DATA *ext_data = get_instance_data(op_data);
    unsigned msg[OPMSG_QVA_MODE_CHANGE_WORD_SIZE];

    L3_DBG_MSG("VA_GM: Configuring QVA");

    OP_CLIENT_MSG_FIELD_SET(msg, OPMSG_QVA_MODE_CHANGE, MESSAGE_ID, OPMSG_QVA_ID_MODE_CHANGE);
    OP_CLIENT_MSG_FIELD_SET(msg, OPMSG_QVA_MODE_CHANGE, WORKING_MODE, mode);

    opmgr_op_client_send_message(op_data, ext_data->qva_op_id,
                                 OPMSG_QVA_MODE_CHANGE_WORD_SIZE,
                                 (unsigned *) &msg);
}

/**
 * \brief Configure CVC into the specified mode.
 *        CVC can work in full processing mode or in passthrough mode.
 *        Note: we only send 1 block in the "set control" message, so as
 *        message length we can use OPMSG_COMMON_SET_CONTROL_WORD_SIZE
 *
 * \param op_data Pointer to the operator instance data.
 * \param mode Operation mode to set CVC.
 */
static void configure_cvc(OPERATOR_DATA *op_data, OPMSG_CVC_SEND_MODE mode)
{
    VAGM_OP_DATA *ext_data = get_instance_data(op_data);
    unsigned msg[OPMSG_COMMON_SET_CONTROL_WORD_SIZE];

    L3_DBG_MSG("VA_GM: Configuring CVC");

    OP_CLIENT_MSG_FIELD_SET(msg, OPMSG_COMMON_SET_CONTROL, MESSAGE_ID, OPMSG_COMMON_ID_SET_CONTROL);
    OP_CLIENT_MSG_FIELD_SET(msg, OPMSG_COMMON_SET_CONTROL, NUM_BLOCKS, 1);
    OP_CLIENT_MSG_FIELD_SET(msg, OPMSG_COMMON_SET_CONTROL, CONTROL_ID, OPMSG_CONTROL_MODE_ID);
    OP_CLIENT_MSG_FIELD_SET(msg, OPMSG_COMMON_SET_CONTROL, VALUE_MSW, 0);
    OP_CLIENT_MSG_FIELD_SET(msg, OPMSG_COMMON_SET_CONTROL, VALUE_LSW, mode);

    opmgr_op_client_send_message(op_data, ext_data->cvc_op_id,
                                 OPMSG_COMMON_SET_CONTROL_WORD_SIZE,
                                 (unsigned *) &msg);
}

/**
 * \brief Handles notification from the Framework that the VA operators have
 *        been delegated to VA Graph Manager. The notification message contains
 *        the operator [external] ids of the VA operators, as seen by the
 *        application client.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
static bool vagm_delegated_ops(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    VAGM_OP_DATA *ext_data = get_instance_data(op_data);
    unsigned msg_len = OPMGR_GET_OPMSG_LENGTH((OP_MSG_REQ *)message_data);

    if (msg_len < OPMSG_OP_CLIENT_DELEGATED_OPERATORS_OPERATORS_WORD_OFFSET
                  + VAGM_MIN_DELEGATED_OPERATORS)
    {
        return FALSE;
    }
    if (msg_len > OPMSG_OP_CLIENT_DELEGATED_OPERATORS_OPERATORS_WORD_OFFSET
                  + VAGM_MAX_DELEGATED_OPERATORS)
    {
        return FALSE;
    }

    vagm_init(op_data);

    L3_DBG_MSG("VA_GM: delegated operators");

    /* Application client will send the VAD operator in the first position. */
    ext_data->vad_op_id = OPMSG_FIELD_GET_FROM_OFFSET(message_data, OPMSG_OP_CLIENT_DELEGATED_OPERATORS, OPERATORS, 0);
    /* Application client will send the VAD operator in the second position. */
    ext_data->qva_op_id = OPMSG_FIELD_GET_FROM_OFFSET(message_data, OPMSG_OP_CLIENT_DELEGATED_OPERATORS, OPERATORS, 1);

    if (msg_len > OPMSG_OP_CLIENT_DELEGATED_OPERATORS_OPERATORS_WORD_OFFSET
                  + VAGM_MIN_DELEGATED_OPERATORS)
    {
        /* Application client will send the CVC operator in the third position. */
        ext_data->cvc_op_id = OPMSG_FIELD_GET_FROM_OFFSET(message_data, OPMSG_OP_CLIENT_DELEGATED_OPERATORS, OPERATORS, 2);
    }

    configure_vad(op_data, OPMSG_VAD_MODE_FULL_PROC);
    configure_qva(op_data, OPMSG_QVA_MODE_PASS_THRU);

    return TRUE;
}

/**
 * \brief Handles notification from the a VA operator that a trigger was
 *        detected.
 *        VAD will send a trigger notification when it recognizes any voice
 *        activity. At that point we configure QVA in full processing mode.
 *        QVA will send a positive trigger notification when it recognizes the
 *        trigger phrase, and we forward this to the application client.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
static bool vagm_trigger(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    VAGM_OP_DATA *ext_data = get_instance_data(op_data);
    OPERATOR_ID src_op_id = OPMSG_FIELD_GET(message_data, OPMSG_OP_CLIENT_UNSOLICITED_MESSAGE, SOURCE_OP_ID);

    if (waiting_for_any_response(ext_data))
    {
        /* A trigger came while waiting for response by operators or aov.
         * This task should run at priority 0, so that the messages
         * are handled sequentially. If this happens, we can ignore it. */
        return TRUE;
    }

    if (src_op_id == ext_data->vad_op_id)
    {
        L3_DBG_MSG("VA_GM: positive trigger from VAD");

        ext_data->voice_activity = TRUE;

        configure_qva(op_data, OPMSG_QVA_MODE_FULL_PROC);
        ext_data->wait_for_qva = TRUE;

        if (ext_data->lp_active)
        {
            /**
             * In low power mode, the MIPS budged is not enough to run QVA.
             * We send a request to the framework to switch to a higher clock.
             */
            if (!manage_clock(op_data, TRUE))
            {
                /* AOV client is not present. Cannot change clock. */
                fault_diatribe(FAULT_AUDIO_VAGM_AOV_NOT_PRESENT, src_op_id);
            }
        }
    }
    else if (src_op_id == ext_data->qva_op_id)
    {
        L3_DBG_MSG("VA_GM: positive trigger from QVA");
        unsigned *payload = OPMSG_FIELD_POINTER_GET(message_data, OPMSG_OP_CLIENT_UNSOLICITED_MESSAGE, PAYLOAD);
        unsigned length = OPMGR_GET_OPCMD_MESSAGE_LENGTH((OPMSG_HEADER*)message_data);
        configure_vad(op_data, OPMSG_VAD_MODE_PASS_THRU);
        configure_qva(op_data, OPMSG_QVA_MODE_PASS_THRU);

        if (ext_data->lp_active)
        {
            /**
             * In low power mode, we need to ask the framework to send a
             * notification to the application client, as the link used by
             * othe application is down.
             */
            if (aov_request_notify_trigger(op_data,
                                           length - CLIENT_UNSOLICITED_MESSAGE_SIZE_EXTRA,
                                           payload))
            {
                ext_data->wait_for_aov = TRUE;

                if (ext_data->cvc_op_id != 0)
                {
                    /* We will now be exiting from low power. */
                    configure_cvc(op_data, OPMSG_CVC_SEND_MODE_FULL_PROC);
                }
            }
            else
            {
                /* AOV client is not present. Cannot change clock. */
                fault_diatribe(FAULT_AUDIO_VAGM_AOV_NOT_PRESENT, src_op_id);
            }
        }
        else
        {
            /**
             * Forward trigger notification and details to the application
             * client through the standard link used by the application.
             */
            common_send_unsolicited_message(op_data, OPMSG_REPLY_ID_VA_TRIGGER,
                                            length - CLIENT_UNSOLICITED_MESSAGE_SIZE_EXTRA,
                                            payload);
        }
    }
    else
    {
        /* Operator id is unrecognized */
        fault_diatribe(FAULT_AUDIO_VAGM_UNRECOGNIZED_OPERATOR, src_op_id);
    }
    return TRUE;
}

/**
 * \brief Handles notification from the a VA operator that a negative trigger
 *        was detected.
 *        QVA will send a negative trigger if the processed audio did not
 *        contain the trigger phrase. In this case we wait for another VAD
 *        trigger.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
static bool vagm_negative_trigger(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    VAGM_OP_DATA *ext_data = get_instance_data(op_data);
    OPERATOR_ID src_op_id = OPMSG_FIELD_GET(message_data, OPMSG_OP_CLIENT_UNSOLICITED_MESSAGE, SOURCE_OP_ID);

    if (waiting_for_any_response(ext_data))
    {
        /* A trigger came while waiting for response by operators or aov.
         * This task should run at priority 0, so that the messages
         * are handled sequentially. If this happens, we can ignore it. */
        return TRUE;
    }

    if (src_op_id == ext_data->qva_op_id)
    {
        L3_DBG_MSG("VA_GM: negative trigger from QVA");
        /* We will ignore this and use the negative trigger from VAD.*/
    }
    else if (src_op_id == ext_data->vad_op_id)
    {
        L3_DBG_MSG("VA_GM: negative trigger from VAD");

        ext_data->voice_activity = FALSE;

        configure_qva(op_data, OPMSG_QVA_MODE_PASS_THRU);
        ext_data->wait_for_qva = TRUE;

        if (ext_data->lp_active)
        {
            /**
             * Since we will wait for another VAD trigger, get back to the
             * default low power clock.
             */
            if (!manage_clock(op_data, FALSE))
            {
                /* AOV client is not present. Cannot change clock. */
                fault_diatribe(FAULT_AUDIO_VAGM_AOV_NOT_PRESENT, src_op_id);
            }
        }
    }
    else
    {
        /* Operator id is unrecognized */
        fault_diatribe(FAULT_AUDIO_VAGM_UNRECOGNIZED_OPERATOR, src_op_id);
    }
    return TRUE;
}

/**
 * \brief Handles a response from a configuration previously send to an operator.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
static bool vagm_message_response_handler(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    VAGM_OP_DATA *ext_data = get_instance_data(op_data);
    OPERATOR_ID src_op_id = OPMSG_FIELD_GET(message_data, OPMSG_OP_CLIENT_MESSAGE_RESPONSE, SOURCE_OP_ID);
    STATUS_KYMERA status = (STATUS_KYMERA) OPMSG_FIELD_GET(message_data, OPMSG_OP_CLIENT_MESSAGE_RESPONSE, STATUS);

    if (status == STATUS_OK)
    {
        if (src_op_id == ext_data->vad_op_id)
        {
            L3_DBG_MSG("VA_GM: response from VAD");
            ext_data->wait_for_vad = FALSE;
        }
        else if (src_op_id == ext_data->qva_op_id)
        {
            L3_DBG_MSG("VA_GM: response from QVA");
            ext_data->wait_for_qva = FALSE;
        }
        else if (src_op_id == ext_data->cvc_op_id)
        {
            L3_DBG_MSG("VA_GM: response from CVC");
            /* We don't need to wait for CVC response. */
        }
        else
        {
            /* Operator id is unrecognized */
            fault_diatribe(FAULT_AUDIO_VAGM_UNRECOGNIZED_OPERATOR, src_op_id);
        }
    }
    else
    {
        /* Operation configuration went wrong */
        fault_diatribe(FAULT_AUDIO_VAGM_ERROR_MESSAGE, src_op_id);
    }
    return TRUE;
}

/**
 * \brief Handles a power state notification from the framework.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
static bool vagm_lp_notification(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    VAGM_OP_DATA *ext_data = get_instance_data(op_data);
    L3_DBG_MSG("VA_GM: Low power notification");
    bool lp_activated = (bool) OPMSG_FIELD_GET(message_data, OPMSG_OP_CLIENT_AOV_LP, ACTIVE);

    if (lp_activated)
    {
        /* We are entering low power. */
        if (ext_data->cvc_op_id != 0)
        {
            /* In low power, we cannot afford to run cvc.*/
            configure_cvc(op_data, OPMSG_CVC_SEND_MODE_PASS_THRU_LEFT);
        }
        if( ext_data->voice_activity)
        {
            /**
             * In low power mode, the MIPS budged is not enough to run QVA.
             * We send a request to the framework to switch to a higher clock.
             */
            if (!manage_clock(op_data, TRUE))
            {
                /* AOV client is not present. Cannot change clock. */
                fault_diatribe(FAULT_AUDIO_VAGM_AOV_NOT_PRESENT, 0);
            }
        }
    }
    else
    {
        if(ext_data->lp_active)
        {
            /* We are entering low power. */
            if (ext_data->cvc_op_id != 0)
            {
                /* We are exiting low power. */
                configure_cvc(op_data, OPMSG_CVC_SEND_MODE_FULL_PROC);
            }
        }
        else
        {
           /* We are still in active mode. */
        }
    }

    ext_data->lp_active = lp_activated;

    return TRUE;

}

/**
 * \brief Handles a response from the framework for a previously sent request.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
static bool vagm_aov_response(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    VAGM_OP_DATA *ext_data = get_instance_data(op_data);

    L3_DBG_MSG("VA_GM: Response from AOV");
    ext_data->wait_for_aov = FALSE;

    return TRUE;
}

/**
 * \brief Sets the MIPS load of the VA graph.
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
static bool vagm_set_graph_load(OPERATOR_DATA *op_data, void *message_data, unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    VAGM_OP_DATA *ext_data = get_instance_data(op_data);

    L3_DBG_MSG("VA_GM: Graph Load");
    ext_data->graph_load = (OPMSG_VA_GM_LOAD) OPMSG_FIELD_GET(message_data, OPMSG_VA_GM_SET_GRAPH_LOAD, LOAD);

    return TRUE;
}


static bool manage_clock(OPERATOR_DATA *op_data, bool custom_clock)
{
    AOV_IF_CPU_CLK clk_val;
    VAGM_OP_DATA *ext_data = get_instance_data(op_data);

    switch(ext_data->graph_load)
    {
    case OPMSG_VA_GM_LOAD_FULL:
        clk_val = AOV_IF_CPU_CLK_TURBO;
        break;
    case OPMSG_VA_GM_LOAD_MEDIUM:
        clk_val = AOV_IF_CPU_CLK_BASE_CLOCK;
        break;
    case OPMSG_VA_GM_LOAD_LOW:
        clk_val = AOV_IF_CPU_CLK_SLOW_CLOCK;
        break;
    default:
        return TRUE;
    }

    L3_DBG_MSG("VA_GM: request AOV");

    if (custom_clock)
    {
        if (!aov_request_custom_clock(op_data, clk_val))
        {
            return FALSE;
        }
    }
    else
    {
        if (!aov_request_default_clock(op_data))
        {
            return FALSE;
        }
    }
    ext_data->wait_for_aov = TRUE;

    return TRUE;
}
