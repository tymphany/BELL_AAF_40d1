/****************************************************************************
 * Copyright 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_sco_napier.c
 * \ingroup stream
 *
 * stream sco type file. <br>
 * This file contains stream functions for napier sco endpoints. <br>
 *
 * \section sec1 Contains:
 * stream_sco_create <br>
 * stream_sco_get_endpoint <br>
 * stream_sco_destroy <br>
 */

/****************************************************************************/
/* Include Files                                                            */
/****************************************************************************/
#include "stream_private.h"
#include "stream_endpoint_sco.h"
#include "sco_mgr_napier.h"
#include "mib/mib.h"

/* #define DEBUG_STREAM_SCO_NAPIER_SOURCE */
/* #define DEBUG_STREAM_SCO_NAPIER_SINK */
#if defined(DEBUG_STREAM_SCO_NAPIER_SOURCE) || defined(DEBUG_STREAM_SCO_NAPIER_SINK)
#include "stdio.h"
#include "stdlib.h"
#endif

/* metadata Sync word */
#define SYNC_WORD ((uint16) (0x5c5c))
/* Byte swap */
#define WBS_METADATA_TRANSFORM(x)    ((x<<8) & 0xFF00) | ((x>>8) & 0x00FF)

/****************************************************************************/
/* Private Type Declarations                                                */
/****************************************************************************/

/****************************************************************************/
/* Private Constant Declarations                                            */
/****************************************************************************/
#define MAX_NUM_RX_BUFFERS  3
#define MAX_NUM_TX_BUFFERS  2
#define SCO_BUFFER_SIZE_NAPIER 256 /* buffer comprises 256 words */
/****************************************************************************/
/* Private Macro Declarations                                               */
/****************************************************************************/
static inline int min(int x,int y){ return ((x<y) ? x : y);}

/****************************************************************************/
/* Private Function Declarations                                            */
/****************************************************************************/
static bool sco_configure_napier(ENDPOINT *endpoint, unsigned int key, uint32 value);
static bool sco_get_config_napier(ENDPOINT *endpoint, unsigned int key, ENDPOINT_GET_CONFIG_RESULT* result);
static sco_metadata_t get_metadata(int* buffer_ptr);
static sco_majority_vote_t sco_ErrDataProcess_MajorityVoting(sco_metadata_t* sco_metadata, int32 buf_num, tCbuffer** rx_buf_ptr, bool voting_is_bypassed);
static unsigned calc_HammingWeight(int num_Bytes, int* rx_buf_ptr0, int* rx_buf_ptr1);
static unsigned calc_QualityMetric_3RxBuffers(int num_Bytes, int* rx_buf_ptr0, int* rx_buf_ptr1,  int* rx_buf_ptr2);

/****************************************************************************/
/* Private Variable Definitions                                             */
/****************************************************************************/
DEFINE_ENDPOINT_FUNCTIONS(sco_functions, sco_close, sco_connect,
                          sco_disconnect, sco_buffer_details,
                          sco_kick, sco_sched_kick, sco_start,
                          sco_stop, sco_configure_napier, sco_get_config_napier,
                          sco_get_timing, stream_sync_sids_dummy,
                          sco_have_same_clock);



/****************************************************************************/
/* PROCESSING                                                               */
/****************************************************************************/

/****************************************************************************/
/* Public Function Definitions                                              */
/****************************************************************************/
/*--------------------------------------------------------------------------*
 * stream_sco_create                                                        *
 *--------------------------------------------------------------------------*/
bool sco_conn_create(unsigned sco_handle, sco_conn_params_t* sco_params)
{
    /* copy those parameters to end format */
    unsigned    tx_buf_size = sco_params->p_stat.tx_buf_size;
    unsigned    tx_buf_num  = sco_params->p_stat.tx_buf_num;
    unsigned    rx_buf_size = sco_params->p_stat.rx_buf_size;
    unsigned    rx_buf_num  = sco_params->p_stat.rx_buf_num;

    uintptr_t   addr_u32;

/*  unsigned    key = sco_handle;       // The hci handle is the key (unique for the type and direction) */
    ENDPOINT    *source_ep = NULL;
    ENDPOINT    *sink_ep = NULL;

    int         k;

    patch_fn_shared(stream_sco);

    PL_PRINT_P1(TR_PL_TEST_TRACE, "\t FUNCTION: bool stream_sco_create(sco_handle=0x%04x) \n", sco_handle);

    /*-------------------*
     * SOURCE_ENDPOINT CHECK: Check that we don't already have an existing source endpoint for the specified hci handle
     *-------------------*/
    if (stream_get_endpoint_from_key_and_functions(sco_handle, SOURCE, &endpoint_sco_functions) != NULL)
    {   /* There is alread an exisitng endpoint: caller should not have called us for a second time without deleting the existing endpoint first */
        goto handle_error;
    }

    /*-------------------*
     * SOURCE_ENDPOINT CREATE: Create and initialise a source endpoint
     *-------------------*/
    source_ep = STREAM_NEW_ENDPOINT(sco, sco_handle, SOURCE, INVALID_CON_ID);
    if (source_ep == NULL)
    {
        goto handle_error;
    }
    /* source ep created */
    source_ep->can_be_closed = FALSE;
    source_ep->can_be_destroyed = FALSE;
    source_ep->is_real = TRUE; /* SCO endpoints are always at the end of a chain */

    /*-------------------*
     * RX BUFFERS linked to SOURCE_EP: setup tCbuffer struct
     *-------------------*/
    source_ep->state.sco.source_buf_num = min(rx_buf_num, MAX_NUM_RX_BUFFERS);  /* up to 3 RX buffers (from-air) */
    source_ep->state.sco.sink_buf_num = 1;                                      /* 1 buffer towards DSP operators */
    addr_u32 = (sco_params->p_stat.rx_buf_ptr);    /* rx_buf_ptr */
    if (rx_buf_size%(1<<LOG2_ADDR_PER_WORD) != 0)  /* request buffer size in Bytes to be integer multiple of buffer size in words */
    {
        goto handle_error; /* buffer size in Bytes not integer multiple of buffer size in words */
    }
    for (k=0; k<source_ep->state.sco.source_buf_num; k++)
    {
        /* given buffer size is in [Bytes] */
        /* allocate memory for tCbuffer struct and initialize pointer and size */
        /*
         * TODO: Is this buffer really a BUF_DESC_IS_REMOTE_MMU_MASK? It is not in a hydra sense, but this descriptor matches
         * the corresponding cbuffer_destroy functionality
         *
         * Note:
         * Given rx_buf_size is in [BYTES]
         * Buffer size function parameter of 'cbuffer_create' function is in words
         * Buffer size stored in tCbuffer struct is in [BYTES]
         * Actually, the size parameter in the tCbuffer struct depends on descriptor (SW_BUFFER: size in 32bit words, MMU_BUFFER size in 16bit words)
         */
        source_ep->state.sco.source_buf[k] = cbuffer_create((void*) addr_u32, (rx_buf_size>>LOG2_ADDR_PER_WORD), BUF_DESC_IS_REMOTE_MMU_MASK); /* considers size in words */
        addr_u32 += rx_buf_size;
        if (source_ep->state.sco.source_buf[k] == NULL)
        {
            goto handle_error; /* something went wrong */
        }
        /* source_ep->state.sco.source_buf[k]->write_ptr = NULL; *//* DSP does not use write pointer of the RX buffer */
    }

    /*-------------------*
     * SINK_ENDPOINT CHECK: Check that we don't already have an existing source endpoint for the specified hci handle
     *-------------------*/
    if (stream_get_endpoint_from_key_and_functions(sco_handle, SINK, &endpoint_sco_functions) != NULL)
    {   /* There is alread an exisitng endpoint: caller should not have called us for a second time without deleting the existing endpoint first */
        goto handle_error;
    }

    /*-------------------*
     * SINK_ENDPOINT CREATE: Create and initialise a source endpoint
     *-------------------*/
    sink_ep = STREAM_NEW_ENDPOINT(sco, sco_handle, SINK, INVALID_CON_ID);
    if (sink_ep == NULL)
    {
        goto handle_error;
    }
    /* sink ep created */
    sink_ep->can_be_closed = FALSE;
    sink_ep->can_be_destroyed = FALSE;
    sink_ep->is_real = TRUE;        /* SCO endpoints are always at the end of a chain */

    /*-------------------*
     * TX BUFFERS linked to SINK_EP: setup tCbuffer struct
     *-------------------*/
    sink_ep->state.sco.sink_buf_num = min(tx_buf_num, MAX_NUM_TX_BUFFERS);  /* up to 2 TX buffers (to-air) */
    sink_ep->state.sco.source_buf_num = 1;                                  /* 1 buffer from DSP operators */
    addr_u32 = (sco_params->p_stat.tx_buf_ptr);    /* tx_buf_ptr */
    if (tx_buf_size%(1<<LOG2_ADDR_PER_WORD) != 0)  /* request buffer size in Bytes to be integer multiple of buffer size in words */
    {
        goto handle_error; /* buffer size in Bytes not integer multiple of buffer size in words */
    }
    for (k=0; k<sink_ep->state.sco.sink_buf_num; k++)
    {   /* given buffer size is in [Bytes] */
        /*
         * TODO: Is this buffer really a BUF_DESC_IS_REMOTE_MMU_MASK? It is not in a hydra sense, but this descriptor matches
         * the corresponding cbuffer_destroy functionality
         *
         *
         * Note:
         * Given tx_buf_size is in [BYTES]
         * Buffer size function parameter of 'cbuffer_create' function is in words
         * Buffer size stored in tCbuffer struct is in [BYTES]
         * Actually, the size parameter in the tCbuffer struct depends on descriptor (SW_BUFFER: size in 32bit words, MMU_BUFFER size in 16bit words)
         */
        sink_ep->state.sco.sink_buf[k] = cbuffer_create((void*) addr_u32, (tx_buf_size>>LOG2_ADDR_PER_WORD), BUF_DESC_IS_REMOTE_MMU_MASK); /* considers size in words */
        addr_u32 += tx_buf_size;
        if (sink_ep->state.sco.sink_buf[k] == NULL)
        {
            goto handle_error; /* something went wrong */
        }
        /* sink_ep->state.sco.sink_buf[k]->read_ptr = NULL; *//* DSP does not use read pointer of the TX buffer */
    }

    /*-------------------*
     * CRREATE SCO LIST ENTRY with linked timing record
     *-------------------*/
    if (!sco_conn_list_add_entry(sco_handle, sco_params)) /* sco_parameters that will be included in the sco list entry for the given sco_handle */
    {   /* an error occured */
        goto handle_error; /* something went wrong */
    }

    /* Initialize measured rate to 1.0*/
    sink_ep->state.sco.rate_measurement = 1 << STREAM_RATEMATCHING_FIX_POINT_SHIFT;
    source_ep->state.sco.rate_measurement = 1 << STREAM_RATEMATCHING_FIX_POINT_SHIFT;

    /* Initialize majority vote */
    /* check MiB key whether majority voting is enabled */
    source_ep->state.sco.majority_vote_bypass = !(mibgetreqbool(SCOMAJORITYVOTEENABLE));
    source_ep->state.sco.majority_vote_questionable_bits_max = 0;

    /* Result: stream_sco_create OK */
    return TRUE;



/*-------------------*/
handle_error:
/*-------------------*/
    /* Cleanup source endpoint and cbuffer if they exist */
    if (source_ep != NULL)
    {
        for (k=0; k<rx_buf_num; k++)
        {
            if (source_ep->state.sco.source_buf[k] != NULL)
            {
                cbuffer_destroy(source_ep->state.sco.source_buf[k]); /* Free up the buffer and associated data space */
            }
        }
        source_ep->can_be_destroyed = TRUE;
        stream_destroy_endpoint(source_ep);
    }

    /* Cleanup sink endpoint and cbuffer if they exist */
    if (sink_ep != NULL)
    {
        for (k=0; k<tx_buf_num; k++)
        {
            if (source_ep->state.sco.sink_buf[k] != NULL)
            {
                cbuffer_destroy(source_ep->state.sco.sink_buf[k]); /* Free up the buffer and associated data space */
            }
        }
        sink_ep->can_be_destroyed = TRUE;
        stream_destroy_endpoint(sink_ep);
    }

    /* Result: stream_sco_create failed */
    PL_PRINT_P0(TR_PL_TEST_TRACE, "\t stream_sco_create failed!!! \n");
    return FALSE;
}


/*--------------------------------------------------------------------------*
 * stream_sco_remove                                                    *
 *--------------------------------------------------------------------------*/
/*
 * sco_params[0] = sco_handle
 */
void sco_conn_remove(unsigned sco_handle)
{

/*  unsigned    key = sco_handle;       // The hci handle is the key (unique for the type and direction) */
    ENDPOINT *ep;

    patch_fn_shared(stream_sco);

    PL_PRINT_P1(TR_PL_TEST_TRACE, "\t FUNCTION: bool stream_sco_destroy(sco_handle=0x%04x) \n", sco_handle);

    /*-------------------*
     * SOURCE_ENDPOINT: Get and close the source endpoint associated with the hci handle
     *-------------------*/
    if ((ep = stream_get_endpoint_from_key_and_functions(sco_handle, SOURCE, &endpoint_sco_functions)) != NULL)
    {   /* exisiting SOURCE endpoint to be destroyed */
        /* clear tCbuffer structs and endpoint for RX/SOURCE */
        ep->can_be_closed = TRUE;
        ep->can_be_destroyed = TRUE;
        stream_close_endpoint(ep);
    }

    /*-------------------*
     * SINK_ENDPOINT: Get and close the sink endpoint associated with the hci handle
     *-------------------*/
    if ((ep = stream_get_endpoint_from_key_and_functions(sco_handle, SINK, &endpoint_sco_functions)) != NULL)
    {   /* exisiting SINK endpoint to be destroyed */
        /* clear tCbuffer structs for TX/SINK */
        ep->can_be_closed = TRUE;
        ep->can_be_destroyed = TRUE;
        stream_close_endpoint(ep);
    }

    /*-------------------*
     * DESTROY SCO LIST ENTRY with linked timing record
     *-------------------*/
    /* Let the sco_mgr_napier know that this endpoint is going away to remove entry in sco connection list */
    PL_PRINT_P0(TR_PL_TEST_TRACE, "\t delete dco list entry... \n");
    if (!sco_conn_list_remove_entry(sco_handle))
    {
        PL_PRINT_P0(TR_PL_TEST_TRACE, "\t ... delete sco list entry failed \n");
    }
    else
    {
        PL_PRINT_P0(TR_PL_TEST_TRACE, "\t ... delete sco list entry done \n");
    }
    PL_PRINT_P0(TR_PL_TEST_TRACE, "\t DONE: stream_sco_destroy(..) \n");
    return;
}


/*--------------------------------------------------------------------------*
 * stream_sco_get_endpoint                                                  *
 *--------------------------------------------------------------------------*/
/*
 *   con_id:     connection ID of the originator of this request
 *   dir:        whether a source or sink is requested
 *   num_params: number of parameters provided
 *   params[0]:  sco_handle
 */
ENDPOINT* stream_sco_get_endpoint(unsigned con_id, ENDPOINT_DIRECTION dir, unsigned num_params, unsigned* params)
{
    ENDPOINT *ep;
    unsigned sco_handle;

    patch_fn_shared(stream_sco);

    PL_PRINT_P0(TR_PL_TEST_TRACE, "\t FUNCTION: ENDPOINT* stream_sco_get_endpoint(..)\n");

    /* Expect reception of 2 parameters:
     * params[0]: INSTANCE; INSTANCE = sco_handle as only parameter in Napier on initial GET_SOURCE/SINK_REQ
     * params[1]: CHANNEL;  Note: CHANNEL parameter need not to be considered in case of SCO
     */
    if (num_params != 2)
    {
        PL_PRINT_P0(TR_PL_TEST_TRACE, "\t ERROR in FUNCTION: ENDPOINT* stream_sco_get_endpoint(..): (num_params != 1) --> return NULL \n");
        return NULL;
    }
    sco_handle = params[0];

    /*-------------------*
     * if ep already exists, get pointer to requested endpoint ID, otherwise NULL
     *-------------------*/
    ep = stream_get_endpoint_from_key_and_functions(sco_handle, dir, &endpoint_sco_functions);
    if (ep)
    {
        /*-------------------*
         * Endpoint is available -> check ID
         *-------------------*/
        if (ep->con_id == INVALID_CON_ID)
        {
            ep->con_id = con_id;
        }
        /* If the client does not own the endpoint they can't access it */
        else if (ep->con_id != con_id)
        {
            PL_PRINT_P0(TR_PL_TEST_TRACE, "\t ERROR in FUNCTION: ENDPOINT* stream_sco_get_endpoint(..): (ep->con_id != con_id) --> return NULL \n");
            return NULL;
        }
    }
    PL_PRINT_P0(TR_PL_TEST_TRACE, "\t DONE: ENDPOINT* stream_sco_get_endpoint(..) \n");
    return ep;
}




/****************************************************************************/
/* Private Function Definitions                                             */
/****************************************************************************/

/*
*--------------------------------------------------------------------------*
* set_wbs_enc_forward_all_kicks_cback                                      *
*--------------------------------------------------------------------------*
*/
static bool set_wbs_enc_forward_all_kicks_cback(unsigned con_id, unsigned status,
    unsigned op_id, unsigned num_resp_params, unsigned *resp_params)
{
    return TRUE;
}


/*
*--------------------------------------------------------------------------*
* set_wbs_enc_forward_all_kicks                                            *
*--------------------------------------------------------------------------*
*/

static void set_wbs_enc_forward_all_kicks(unsigned int src_id, bool enabled)
{
    /* Create a message and send it to the WBS_ENC operator */
    unsigned params[2];
    patch_fn_shared(stream_sco);

    params[0] = OPMSG_WBS_ENC_ID_FORWARD_ALL_KICKS;
    params[1] = enabled;

    opmgr_operator_message(OPMGR_ADAPTOR_OBPM, src_id, sizeof(params) / sizeof(unsigned),
        params, set_wbs_enc_forward_all_kicks_cback);
}


/*--------------------------------------------------------------------------*
 * sco_close                                                                *
 *--------------------------------------------------------------------------*/
bool sco_close(ENDPOINT *endpoint, bool *pending)
{
    int k;

    patch_fn_shared(stream_sco);

    /* When reaching this function we will have stopped everything from running, so all we need to do is tidy up the buffer */
    if (endpoint->direction == SOURCE)
    {   /* SOURCE */
        for (k=0; k<endpoint->state.sco.source_buf_num; k++)
        {
            cbuffer_destroy(endpoint->state.sco.source_buf[k]);
        }
    }
    else
    {   /* SINK */
        for (k=0; k<endpoint->state.sco.sink_buf_num; k++)
        {
            cbuffer_destroy(endpoint->state.sco.sink_buf[k]);
        }
    }
    return TRUE;
}


/*--------------------------------------------------------------------------*
 * sco_connect                                                              *
 *--------------------------------------------------------------------------*/
bool sco_connect(ENDPOINT *endpoint, tCbuffer *Cbuffer_ptr, ENDPOINT *ep_to_kick, bool* start_on_connect)
{   /* this function concerns the buffers btw. EP and operators */

    patch_fn_shared(stream_sco);

    endpoint->ep_to_kick = ep_to_kick;
    if (endpoint->direction == SOURCE)
    {
        /* Remember this buffer so that the endpoint can write into it
         * when it gets kicked */
        endpoint->state.sco.sink_buf[0] = Cbuffer_ptr; /* there is only a single buffer btw. SCO_EP and operators to connect */
        /* Make a record of the SCO source endpoint in the SCO connection list structure for first kick scheduling calculations */
        sco_from_air_endpoint_set(stream_sco_get_hci_handle(endpoint), endpoint);
    }
    else
    {
        /* Remember this buffer so that the endpoint can read from it
         * when it gets kicked */
        endpoint->state.sco.source_buf[0] = Cbuffer_ptr; /* there is only a single buffer btw. SCO_EP and operators to connect */
        /* Make a record of the SCO sink endpoint in the SCO connection list structure for first kick scheduling calculations */
        sco_to_air_endpoint_set(stream_sco_get_hci_handle(endpoint), endpoint);
    }
    *start_on_connect = FALSE;


    /*
    * Napier: configure WBS_ENC to forward kick to SCO_SINK_EP on Tesco rate
    * to support 30Bytes/Tesco=6 (see B-245212).
    */
    if (opmgr_get_op_capid(stream_ep_id_from_ep(endpoint->connected_to)) == CAP_ID_WBS_ENC)
    {
        set_wbs_enc_forward_all_kicks(endpoint->connected_to->id, TRUE);
    }

    return TRUE;
}


/*--------------------------------------------------------------------------*
 * sco_buffer_details                                                       *
 *--------------------------------------------------------------------------*/
bool sco_buffer_details(ENDPOINT *endpoint, BUFFER_DETAILS *details)
{   /* this function is related to SCO_ep_BUFFER towards OPERATOR */

    patch_fn_shared(stream_sco);

    /* this buffer is SW buffers */
    if (endpoint == NULL || details == NULL)
    {
        return FALSE;
    }

    /* when using different in and output buffers in endpoint endpoint set the buffer size of the buffer towards operators to fix size. */
    details->b.buff_params.size = SCO_BUFFER_SIZE_NAPIER;
    details->b.buff_params.flags = BUF_DESC_SW_BUFFER; /* for SW buffers the size is in words */

    /* when buffer is already allocated prior connect and already available (supplies_buffer=TRUE); when buffer is allocated while connect (supplies_buffer=FALSE) */
    details->supplies_buffer = FALSE;
    details->runs_in_place = FALSE;

    return TRUE;
}


/*--------------------------------------------------------------------------*
 * sco_set_data_format                                                      *
 *--------------------------------------------------------------------------*/
bool sco_set_data_format (ENDPOINT *endpoint, AUDIO_DATA_FORMAT format)
{   /* data format of buffers bzw. EP and Operators */

    patch_fn_shared(stream_sco);

    /*
     * NOTE:
     *      In Hydra, the data format is derived from the configuration settings 'cbuffer_set_write_shift' and 'cbuffer_set_write_byte_swap'
     *      Due to the in-place processing in Hydra the related buffers are already available before the 'sco_set_data_format' is called.
     *      However, by default the 'sco_set_data_format' function is called before the buffers are allocated while connecting.
     *      As a consequence, the 'endpoint->state.sco.source/sink_buf pointer are NULL pointers at the time of the 'sco_set_data_format' call
     *      causing a risk to overwrite an undetermined memory location.
     *      Accordingly, in Napier 'cbuffer_set_write_shift' and 'cbuffer_set_write_byte_swap' could not be used here (as e.g. done in Hydra -
     *      Note that Napier does not apply cbops processing whereas in Hydra these setting are applied in the cbops)
     *
     */

    /* The data format can only be set before connect */
    if (NULL != endpoint->connected_to)
    {
        return FALSE;
    }

    /* Sources and sinks have different data formats due to metadata */
    if(stream_direction_from_endpoint(endpoint) == SOURCE)
    {
        /*-------------------*
         * SOURCE
         *-------------------*/
        /* variable 'format' carries the DATA_FORMAT of the input buffer terminal of the connected operator sink --> check if this format is supported by endpoint */
        switch(format)
        {   /* this source ep could be connected to operators, which have an input buffer terminals of the following DATA_FORMAT_TYPES: */
            case AUDIO_DATA_FORMAT_16_BIT_WITH_METADATA:
                 /* SCO_NB: CVSD_DEC operator input terminal */
                 endpoint->state.sco.audio_data_format = AUDIO_DATA_FORMAT_16_BIT_WITH_METADATA;
                 return TRUE;

            case AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP_WITH_METADATA:
                 /* SCO_WBS: WBS_DEC operator input terminal */
                 endpoint->state.sco.audio_data_format = AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP_WITH_METADATA;
                 return TRUE;

            default: /* DATA FORMAT is not supported by endpoint */
                 return FALSE;
        }
    }
    else
    {
        /*-------------------*
         * SINK
         *-------------------*/
        switch(format)
        {   /* this sink ep could be connected to operators, which have an output buffer terminals of the following DATA_FORMAT_TYPES: */
            /* this the input format, feed by CVSD_ENCODER.
             * Despite of that term, CVSD delivers pure data and not metadata counter.
             * Metadata counter is added in SCO_SINK_EP
             */
            case AUDIO_DATA_FORMAT_16_BIT:
                 /* SCO_NB: CVSD_ENC operator output terminal */
                 endpoint->state.sco.audio_data_format = AUDIO_DATA_FORMAT_16_BIT;
                 return TRUE;

            case AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP:
                 /* SCO_WBS: WBS_ENC operator output terminal */
                 endpoint->state.sco.audio_data_format = AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP;
                 return TRUE;

            default:
                 return FALSE;
        }
    }
}



/*--------------------------------------------------------------------------*
 * sco_get_data_format                                                      *
 *--------------------------------------------------------------------------*/
AUDIO_DATA_FORMAT sco_get_data_format (ENDPOINT *endpoint)
{

     patch_fn_shared(stream_sco);

#if 0
    /* 'cbuffer_get_write_byte_swap(..)', 'cbuffer_get_read_byte_swap(..)' not available
     *
     * Note:
     * In BC, Hydra, audio format is distinguished using the byte swap or shift indication
     * In Napier: AUDIO_DATA_FORMAT_16_BIT_WITH_METADATA and AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP_WITH_METADATA
     * could only be distinguished using the byte swap indication, because the shift indication is the same.
     * However, the cbuffer functions do currently not provide a read function for the byte swap indication.
     * That's why in napier, the audio data format is directly written to the endpoint sco struct
     */
    if(stream_direction_from_endpoint(endpoint) == SOURCE)
    {
         if (cbuffer_get_write_byte_swap(endpoint->state.sco.sink_buf[0]) == FALSE)
         {   /* SCO_NB: CVSD_DEC operator input terminal */
             return AUDIO_DATA_FORMAT_16_BIT_WITH_METADATA;
         }
         /* otherwise: */
         /* SCO_WBS: WBS_DEC operator input terminal */
         return AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP_WITH_METADATA;
    }
    else
    {
         if (cbuffer_get_read_byte_swap(endpoint->state.sco.source_buf[0]) == FALSE)
         {   /* SCO_NB: CVSD_ENC operator output terminal */
             return AUDIO_DATA_FORMAT_16_BIT;
         }
         /* otherwise: */
         /* SCO_WBS: WBS_ENC operator output terminal */
         return AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP;
    }
#endif

    return (endpoint->state.sco.audio_data_format);


}


/*--------------------------------------------------------------------------*
 * sco_get_timing                                                           *
 *--------------------------------------------------------------------------*/
void sco_get_timing (ENDPOINT *endpoint, ENDPOINT_TIMING_INFORMATION *time_info)
{
    patch_fn_shared(stream_sco);

    sco_common_get_timing(endpoint, time_info);

    if (endpoint->direction == SOURCE)
    {
        time_info->wants_kicks = FALSE;
    }
    else
    {
        time_info->wants_kicks = TRUE;
    }
}


/*--------------------------------------------------------------------------*
 * sco_disconnect                                                           *
 *--------------------------------------------------------------------------*/
bool sco_disconnect(ENDPOINT *endpoint)
{   /* this function concerns the buffers btw. EP and operators */
    int k;

    patch_fn_shared(stream_sco);

    /* TODO: do we need to stop any scheduling at this time? */
    if (endpoint->direction == SOURCE)
    {   /* SOURCE */
        for (k=0; k<endpoint->state.sco.sink_buf_num; k++)
        {
            endpoint->state.sco.sink_buf[k] = NULL;
        }
    }
    else
    {   /* SINK */
        for (k=0; k<endpoint->state.sco.source_buf_num; k++)
        {
            endpoint->state.sco.source_buf[k] = NULL;
        }
    }
    return TRUE;
}


/*--------------------------------------------------------------------------*
 * sco_kick                                                                 *
 *--------------------------------------------------------------------------*/
void sco_kick(ENDPOINT *endpoint, ENDPOINT_KICK_DIRECTION kick_dir)
{
    int             k;
    int             ScoPacketSize_Bytes;
    int             word16_2tuple[2];
    endpoint_sco_state  *sco_state = &(endpoint->state.sco);

    patch_fn_shared(stream_sco);

    /* if it is a source then we need to kick the next endpoint in the chain */
    /* if it is a sink we are the end of the chain and there is nothing more to do */
    if (endpoint->direction == SOURCE)
    {
        sco_metadata_t                metadata_rx[MAX_NUM_RX_BUFFERS]; /* RX BUF_1..3 */
        unsigned                      tesco_bt_ticks = (sco_tesco_get(stream_sco_get_hci_handle(endpoint))<<1)/US_PER_SLOT;
        int                           metadata_word16[5];      /* 5 WORD16 metadata */
        sco_majority_vote_t           bufVote;

        /**********************************/
        /*    SCO EP SOURCE PROCESSING    */
        /**********************************/
        /*
         * NOTE:
         *       We find the metadata always on the same address of the RX buffer
         *       Except from the CRC indication, we expect same metadata in all Rx buffers
         */
        for (k=0; k<sco_state->source_buf_num; k++)
        {
             metadata_rx[k] = get_metadata(sco_state->source_buf[k]->read_ptr); /* RX BUF_1..3 */
        }

        /*
         *************************************************************
         * Select buffer Idx or enable majority voting if applicable...
         *
         * STATUS as delivered by M0 via in-band metadata:
         *           RX_BUFFER_STATUS_NONE    = 0: No AC detect or Packet Header Checksum Failure
         *           RX_BUFFER_STATUS_VALID   = 1: Valid SCO packet or eSCO with good CRC
         *           RX_BUFFER_STATUS_BAD_CRC = 2: eSCO packet with CRC failure
         *************************************************************
         * If status of selected buffer is RX_BUFFER_STATUS_NONE, we don't have new SCO PACKET -> apply PLC: handled in subsequent SCO_RCV resp. WBS_DEC
         * metadata.counter and metadata.status are used as they are...
         */
         bufVote = sco_ErrDataProcess_MajorityVoting(metadata_rx, sco_state->source_buf_num,
                                                        sco_state->source_buf, sco_state->majority_vote_bypass);

        /*
         *---------------------------------------------------------------*
         *           backup metadata_len and payload_size                *
         *---------------------------------------------------------------*
         */
        /* metadata counter is updated in each case (all buffers have same counter value) */
        /* in case of status = RX_BUFFER_STATUS_NONE, we assume the values from the last valid sco packet for the remaining metadata */
        if (metadata_rx[bufVote.bufIdx].status == RX_BUFFER_STATUS_NONE)
        {   /* use last useable metadata.metadata_len and payload instead */
            metadata_rx[bufVote.bufIdx].metadata_len = sco_state->metadata_backup_len;
            metadata_rx[bufVote.bufIdx].payload_size = sco_state->metadata_backup_payload_size;
        }
        else
        {   /* RX_BUFFER_STATUS_VALID or RX_BUFFER_STATUS_BAD_CRC */
            sco_state->metadata_backup_len = metadata_rx[bufVote.bufIdx].metadata_len; /* store last useable metadata_len */
            sco_state->metadata_backup_payload_size = metadata_rx[bufVote.bufIdx].payload_size; /* store last useable payload_size */
        }
        /* in absence of any previous valid metadata, there's nothing to do... */
        if (metadata_rx[bufVote.bufIdx].metadata_len == 0)
        {
            return;
        }

        /*
         *-------------------------------------------------------------------------------*
         * check whether the delivery would fit in the associated buffers in [Bytes]     *
         * Note: all buffers have same metadata_len, payload-size and buffer size value  *
         *-------------------------------------------------------------------------------*
         */
        ScoPacketSize_Bytes = (metadata_rx[bufVote.bufIdx].metadata_len + metadata_rx[bufVote.bufIdx].payload_size); /* SCO_SOURCE: packet_size = metadata_size + payload_size   */
        if (  ((ScoPacketSize_Bytes + sizeof(SYNC_WORD)) > sco_state->sink_buf[0]->size)                    /* remind: We have to add 2 Bytes due to SyncWord to output */
           || ( ScoPacketSize_Bytes      > sco_state->source_buf[bufVote.bufIdx]->size))
        {  /* ERROR: SCO packet delivery does not fit in associated buffers */
           return;
        }


        /*
         ********************************
         * SCO_SOURCE: COPY SCO_PACKET  FROM IN- to OUT buffer
         *
         * Transform metadata (received from M0 and included in RX buffer)
         * to metadata format as used by BC/Hydra to reuse subsequent capabilities,
         * as e.g. WBS_DEC, SCO_RCV
         *
         * SCO_SOURCE_EP DATA_FORMAT@SINK_BUF:
         * SCO_NB:  AUDIO_DATA_FORMAT_16_BIT_WITH_METADATA
         * SCO_WBS: AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP_WITH_METADATA
         ********************************
         */

        /*
         *-------------------------------------------------------------------------------*
         * WRITE METADATA (5 WORD16 (=10 BYTES))        *
         *         WORD16[0]: METADATA_SYNC_WORD        *
         *         WORD16[0]: METADATA_METADATA_LENGTH  *
         *         WORD16[2]: METADATA_PAYLOAD_SIZE     *
         *         WORD16[3]: METADATA_STATUS           *
         *         WORD16[4]: METADATA_TIMESTAMP        *
         *-------------------------------------------------------------------------------*
         * Note to METADATA_STATUS
         * Hydra and BC consider metadata.status in sco_nb and sco_wb as follows (--> see sco_struct.h in capabilities/sco_fw/), which is different from RX_BUFFER_STATUS we got from M0.
         * --> we have to map the status we got from M0 to the expected status of subsequent capabilities
         *
         * check quality meassure in case of MajorityVoting && RX_BUFFER_STATUS_BAD_CRC
         * in case of 'RX_BUFFER_STATUS_BAD_CRC' and MajorityVoting and less Bits of current SCO packet are unreliable than given thrshld after majority voting,
         * we overwrite the status: SCO_PACKET_STATUS_CRC_ERROR by SCO_PACKET_STATUS_OK
         *
         *
         * Note to METADATA_TIMESTAMP:
         * What we got along with the M0 metadata is just a counter....
         *
         * Actually, the SCO_RCV and SCO_WBS expect a timestamp in BT ticks.
         * Time btw. 2 master slots in [BT_TICKS] is 2*Tesco if Tesco is given in [SLOTS]
         *
         * However, the available timing parameters provide Tesco only in [us] -> this would mean: TIME_SPAN [BT_TICKS] = Tesco [us] / 312.5 [us]
         * The Tesco in [us] we could get from
         * Tesco [us] = endpoint->state.sco.kick_period>>6;
         * Tesco [us] = sco_tesco_get(stream_sco_get_hci_handle(endpoint));
         *
         * We apply a "counter-to-timestamp" conversion
         */
         if (sco_get_data_format(endpoint) == AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP_WITH_METADATA)
         {
             metadata_word16[METADATA_SYNC_WORD_IDX]       = WBS_METADATA_TRANSFORM(SYNC_WORD);
             metadata_word16[METADATA_METADATA_LENGTH_IDX] = WBS_METADATA_TRANSFORM((metadata_rx[bufVote.bufIdx].metadata_len + sizeof(SYNC_WORD))>>1); /* Length in WORD16 */
             metadata_word16[METADATA_PAYLOAD_SIZE_IDX]    = WBS_METADATA_TRANSFORM(metadata_rx[bufVote.bufIdx].payload_size);                          /* Size in BYTES */
             metadata_word16[METADATA_STATUS_IDX]          = (metadata_rx[bufVote.bufIdx].status == RX_BUFFER_STATUS_VALID)   ? WBS_METADATA_TRANSFORM((uint16) SCO_PACKET_STATUS_OK)        :
                                                             (metadata_rx[bufVote.bufIdx].status == RX_BUFFER_STATUS_BAD_CRC) ? WBS_METADATA_TRANSFORM((uint16) SCO_PACKET_STATUS_CRC_ERROR) :
                                                                                                                                WBS_METADATA_TRANSFORM((uint16) SCO_PACKET_STATUS_NOTHING_RECEIVED);
             metadata_word16[METADATA_TIMESTAMP_IDX]       = WBS_METADATA_TRANSFORM(((sco_state->time_stamp_init + (tesco_bt_ticks * metadata_rx[bufVote.bufIdx].counter)) & 0xFFFF));

             /* check quality meassure in case of MajorityVoting && RX_BUFFER_STATUS_BAD_CRC */
             if (metadata_rx[bufVote.bufIdx].status == RX_BUFFER_STATUS_BAD_CRC)
             {
                 if ((sco_state->majority_vote_bypass == FALSE) && (bufVote.quality_metric <= sco_state->majority_vote_questionable_bits_max))
                 {
                       metadata_word16[METADATA_STATUS_IDX] = WBS_METADATA_TRANSFORM((uint16) SCO_PACKET_STATUS_OK); /* overwrite the status */
                 }
             }
         }
         else
         {
             metadata_word16[METADATA_SYNC_WORD_IDX]       = SYNC_WORD;
             metadata_word16[METADATA_METADATA_LENGTH_IDX] = (metadata_rx[bufVote.bufIdx].metadata_len + sizeof(SYNC_WORD))>>1;
             metadata_word16[METADATA_PAYLOAD_SIZE_IDX]    = (metadata_rx[bufVote.bufIdx].payload_size);
             metadata_word16[METADATA_STATUS_IDX]          = (metadata_rx[bufVote.bufIdx].status == RX_BUFFER_STATUS_VALID)   ? SCO_PACKET_STATUS_OK        :
                                                             (metadata_rx[bufVote.bufIdx].status == RX_BUFFER_STATUS_BAD_CRC) ? SCO_PACKET_STATUS_CRC_ERROR :
                                                                                                                                SCO_PACKET_STATUS_NOTHING_RECEIVED;
             metadata_word16[METADATA_TIMESTAMP_IDX]       = ((sco_state->time_stamp_init + (tesco_bt_ticks * metadata_rx[bufVote.bufIdx].counter)) & 0xFFFF);

             /* check quality meassure in case of MajorityVoting && RX_BUFFER_STATUS_BAD_CRC */
             if (metadata_rx[bufVote.bufIdx].status == RX_BUFFER_STATUS_BAD_CRC)
             {
                 if ((sco_state->majority_vote_bypass == FALSE) && (bufVote.quality_metric <= sco_state->majority_vote_questionable_bits_max))
                 {
                       metadata_word16[METADATA_STATUS_IDX] = SCO_PACKET_STATUS_OK; /* overwrite the status */
                 }
             }
         }
         cbuffer_write(sco_state->sink_buf[0], &metadata_word16[0], 5);  /* write metadata_word16 to buffer (5 x WORD16 ) */



        /*
         *****************************
         * NOTE:
         * The data buffer interface towards the capabilities considers 32bit words (SCO_SOURCE_EP_SINK_BUFFER)
         * Keep in mind, that only 16 bits are transferred per 32-bit word.
         * In contrast to that, RX buffer as read by SCO_SOURCE_EP (and fed by M0) provides 32 bit per 32-bit word.
         *
         * To sink buffer we write 16 encoded bits per addressed 32-bit word (NB: AUDIO_DATA_FORMAT_16_BIT_WITH_METADATA,  WBS: AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP_WITH_METADATA)
         *
         * Note concerning DATA_FORMAT:
         * Observations on a given WBS unit test has shown, that the BYTE swap was obviously done differently in GORDON and AMBER
         *                                              WORD16[0..4]      WORD16[ELSE]         RESULT
         * rep_sp_wb_meta.dat (applied for gordon):     SWAP              NO                   WBS_DEC: INVALID
         *                                              SWAP              SWAP                 (OK) @ 8kHz
         * comp_dist_wb_meta.dat (applied for amber)    SWAP              NO                   OK @ 16kHz
         *                                              SWAP              SWAP                 INVALID
         *
         * FOR NAPIER, we apply BYTE swap as considered in WBS_DEC operator on data as well as on payload
         *
         *-------------------------------------------------------------------------------*
         * WRITE PAYLOAD                                                                 *
         *-------------------------------------------------------------------------------*
         */

        int PayloadSizeAndMetadata_word32 = ScoPacketSize_Bytes>>2;          /* Amount of complete word32: METADATA+PAYLOAD w/o SYNC*/
        int PayloadSizeAndMetadata_FracWord32_bytes = ScoPacketSize_Bytes%4; /* BYTES in remaining fract. word32 */
        int word32;

        /*
         *-----------------------------------------------------*
         * AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP_WITH_METADATA    *
         *-----------------------------------------------------*
         */
        int* source_buf0_read_ptr = sco_state->source_buf[0]->read_ptr;
        int* source_buf1_read_ptr = sco_state->source_buf[1]->read_ptr;
        int* source_buf2_read_ptr = sco_state->source_buf[2]->read_ptr;
        if (sco_get_data_format(endpoint) == AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP_WITH_METADATA)
        {   /* process complete word32 */
            if (bufVote.isVoting)
            {   /* apply majority voting (2 out of 3 voting) */
                for (k=2; k<PayloadSizeAndMetadata_word32; k++) /* 1st two word32 in RX buffer are metadata and assigned already above */
                {
                    word32   = ((source_buf0_read_ptr[k])) & ((source_buf1_read_ptr[k]));
                    word32  |= ((source_buf1_read_ptr[k])) & ((source_buf2_read_ptr[k]));
                    word32  |= ((source_buf0_read_ptr[k])) & ((source_buf2_read_ptr[k]));
                    word16_2tuple[0] = WBS_METADATA_TRANSFORM((int)  (word32      & 0xFFFF));  /* write LSWORD16 */
                    word16_2tuple[1] = WBS_METADATA_TRANSFORM((int) ((word32>>16) & 0xFFFF));  /* write MSWORD16 */
                    cbuffer_write(sco_state->sink_buf[0], &word16_2tuple[0], 2);      /* write word16 to buffer */
                }
            }
            else
            {   /* use bufVote.bufIdx */
                for (k=2; k<PayloadSizeAndMetadata_word32; k++) /* 1st two word32 in RX buffer are metadata and assigned already above */
                {
                    word32 = sco_state->source_buf[bufVote.bufIdx]->read_ptr[k];    /* deliver 16 bits per WORD32 */
                    word16_2tuple[0] = WBS_METADATA_TRANSFORM((int)  (word32      & 0xFFFF));  /* write LSWORD16 */
                    word16_2tuple[1] = WBS_METADATA_TRANSFORM((int) ((word32>>16) & 0xFFFF));  /* write MSWORD16 */
                    cbuffer_write(sco_state->sink_buf[0], &word16_2tuple[0], 2);      /* write word16 to buffer */
                }
            }

            /* process fractional word32 */
            if (PayloadSizeAndMetadata_FracWord32_bytes > 0)
            {
                if (bufVote.isVoting)
                {   /* apply majority voting (2 out of 3 voting) */
                    word32   = ((source_buf0_read_ptr[k])) & ((source_buf1_read_ptr[k]));
                    word32  |= ((source_buf1_read_ptr[k])) & ((source_buf2_read_ptr[k]));
                    word32  |= ((source_buf0_read_ptr[k])) & ((source_buf2_read_ptr[k]));
                }
                else
                {   /* use bufVote.bufIdx */
                    word32 = sco_state->source_buf[bufVote.bufIdx]->read_ptr[k];  /* read remaining frac. WORD32 */
                }
                if (PayloadSizeAndMetadata_FracWord32_bytes== 1)
                {
                    word16_2tuple[0] = WBS_METADATA_TRANSFORM(((int) (word32 & 0x00FF)));
                    cbuffer_write(sco_state->sink_buf[0], &word16_2tuple[0], 1);      /* write word16 to buffer (write LSB) */
                }
                else if (PayloadSizeAndMetadata_FracWord32_bytes== 2)
                {
                    word16_2tuple[0] = WBS_METADATA_TRANSFORM(((int) (word32 & 0xFFFF)));
                    cbuffer_write(sco_state->sink_buf[0], &word16_2tuple[0], 1);      /* write word16 to buffer (write LSB) */
                }
                else if (PayloadSizeAndMetadata_FracWord32_bytes== 3)
                {
                    word16_2tuple[0] = WBS_METADATA_TRANSFORM((int)  (word32      & 0xFFFF));  /* write LSWORD16 */
                    word16_2tuple[1] = WBS_METADATA_TRANSFORM((int) ((word32>>16) & 0x00FF));  /* write MSWORD16 */
                    cbuffer_write(sco_state->sink_buf[0], &word16_2tuple[0], 2);      /* write word16 to buffer (write LSB) */
                }
            }
        } /* end of AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP_WITH_METADATA */

        /*
         *-----------------------------------------------------*
         * AUDIO_DATA_FORMAT_16_BIT_WITH_METADATA         *
         *-----------------------------------------------------*
         */
         else
         {  /* process complete word32 */
            if (bufVote.isVoting)
            {   /* apply majority voting (2 out of 3 voting) */
                for (k=2; k<PayloadSizeAndMetadata_word32; k++) /* 1st two word32 in RX buffer are metadata and assigned already above */
                {
                      word32   = ((source_buf0_read_ptr[k])) & ((source_buf1_read_ptr[k]));
                      word32  |= ((source_buf1_read_ptr[k])) & ((source_buf2_read_ptr[k]));
                      word32  |= ((source_buf0_read_ptr[k])) & ((source_buf2_read_ptr[k]));
                      word16_2tuple[0] = (int)  (word32      & 0xFFFF);  /* write LSWORD16 */
                      word16_2tuple[1] = (int) ((word32>>16) & 0xFFFF);  /* write MSWORD16 */
                      cbuffer_write(sco_state->sink_buf[0], &word16_2tuple[0], 2);  /* write word16 to buffer */
                }
            }
            else
            {   /* use bufVote.bufIdx */
                for (k=2; k<PayloadSizeAndMetadata_word32; k++) /* 1st two word32 in RX buffer are metadata and assigned already above */
                {
                    word32 = sco_state->source_buf[bufVote.bufIdx]->read_ptr[k]; /* deliver 16 bits per WORD32 */
                    word16_2tuple[0] = (int)  (word32      & 0xFFFF);  /* write LSWORD16 */
                    word16_2tuple[1] = (int) ((word32>>16) & 0xFFFF);  /* write MSWORD16 */
                    cbuffer_write(sco_state->sink_buf[0], &word16_2tuple[0], 2);   /* write word16 to buffer */
                }
            }

            /* process fractional word32 */
            if (PayloadSizeAndMetadata_FracWord32_bytes > 0)
            {
                if (bufVote.isVoting)
                {   /* apply majority voting (2 out of 3 voting) */
                    word32   = ((source_buf0_read_ptr[k])) & ((source_buf1_read_ptr[k]));
                    word32  |= ((source_buf1_read_ptr[k])) & ((source_buf2_read_ptr[k]));
                    word32  |= ((source_buf0_read_ptr[k])) & ((source_buf2_read_ptr[k]));
                }
                else
                {   /* use bufVote.bufIdx */
                    word32 = sco_state->source_buf[bufVote.bufIdx]->read_ptr[k]; /* read remaining frac. WORD32 */
                }
                if (PayloadSizeAndMetadata_FracWord32_bytes == 1)
                {
                    word16_2tuple[0] = ((int) (word32 & 0x00FF));
                    cbuffer_write(sco_state->sink_buf[0], &word16_2tuple[0], 1);  /* write word16 to buffer (write LSB) */
                }
                else if (PayloadSizeAndMetadata_FracWord32_bytes == 2)
                {
                    word16_2tuple[0] = ((int) (word32 & 0xFFFF));
                    cbuffer_write(sco_state->sink_buf[0], &word16_2tuple[0], 1);  /* write word16 to buffer (write LSB) */
                }
                else if (PayloadSizeAndMetadata_FracWord32_bytes == 3)
                {
                    word16_2tuple[0] = (int)  (word32      & 0xFFFF);  /* write LSWORD16 */
                    word16_2tuple[1] = (int) ((word32>>16) & 0x00FF);  /* write MSWORD16 */
                    cbuffer_write(sco_state->sink_buf[0], &word16_2tuple[0], 2);  /* write word16 to buffer (write LSB) */
                }
            }
        } /* end of AUDIO_DATA_FORMAT_16_BIT_WITH_METADATA */

        /*
         *------------------------*
         * kick the conected Op   *
         *------------------------*
         */
        propagate_kick(endpoint, STREAM_KICK_INTERNAL);


#ifdef DEBUG_STREAM_SCO_NAPIER_SOURCE
        static unsigned counter_sco_src;
        int             NumOutSamples = (ScoPacketSize_Bytes + 2 + 1)>>1; /* Number of delivered WORD16 samples */
        printf ("\n");
        printf ("********************************************************\n");
        printf ("*** file:     components/stream/stream_sco_napier.c  ***\n");
        printf ("*** function: 'sco_kick'                             ***\n");
        printf ("*** SCO_SOURCE_EP is KICKING.....                    ***\n");
        printf ("********************************************************\n");
        printf ("\n");
        printf(" AUDIO_DATA_FORMAT (connected OP)=%02d \n", sco_get_data_format(endpoint));
        printf("\n");
        printf ("************ METADATA ************\n");
        for (k=0; k<endpoint->state.sco.source_buf_num; k++)
        {
            printf(" METADATA RX_BUF[%02d]: BufSize=0x%02X [BYTES] \t metadata_len=0x%02X [BYTES] \t status=0x%02X \t payload_size=0x%02X [BYTES] \t counter=%04d [BYTES]\n",
                     k, endpoint->state.sco.source_buf[k]->size, metadata_rx[k].metadata_len, metadata_rx[k].status, metadata_rx[k].payload_size, metadata_rx[k].counter);
        }
        printf("\n");
        printf ("******** MAJORITY VOTING *********\n");
        printf(" MAJORITY_VOTE_BYPASS=%02d: MAJORITY VOTING applied=%02d   or   SELECTION of RX_BUF[%02d]     BUFFER STATUS=%02d (0:STATUS_NONE  1:STATUS_VALID  2:STATUS_BAD_CRC) \n",
                 endpoint->state.sco.majority_vote_bypass, bufVote.isVoting, bufVote.bufIdx, metadata_rx[bufVote.bufIdx].status);
        printf(" QUALITY METRIC: UNRELIABLE-TO-TOTAL BITS per SCO PACKET = %02d [BITS]/ %03d [BITS] \n", bufVote.quality_metric, 8*metadata_rx[bufVote.bufIdx].payload_size);
        printf("\n");
        printf ("******** TIMING *********\n");
        printf(" time_now = %04d [us] \t SCO_SOURCE_COUNTER = %02d \n", time_get_time(), counter_sco_src);
        printf(" INIT_TIMESTAMP = 0x%04X \t\t tesco = %d [BT-TICKS] \n", endpoint->state.sco.time_stamp_init, tesco_bt_ticks);
        printf(" Current WALLCLOCK value = 0x%4X \n", sco_wallclock_get(stream_sco_get_hci_handle(endpoint)));
        printf("\n");
        printf ("******** SAMPLES *********\n");
        printf(" SAMPLES @ INPUT \n");
        for (k=0; k<endpoint->state.sco.source_buf[0]->size>>LOG2_ADDR_PER_WORD; k++)
        {
             printf(" RX_BUF_0[%2d]: ADDR=0x%08X / VALUE=0x%08X    ", k, endpoint->state.sco.source_buf[0]->read_ptr + k, endpoint->state.sco.source_buf[0]->read_ptr[k]);
             if (endpoint->state.sco.source_buf_num > 1)
             {
                 printf(" RX_BUF_1[%2d]: ADDR=0x%08X / VALUE=0x%08X    ", k, endpoint->state.sco.source_buf[1]->read_ptr + k, endpoint->state.sco.source_buf[1]->read_ptr[k]);
             }
             if (endpoint->state.sco.source_buf_num > 2)
             {
                 printf(" RX_BUF_2[%2d]: ADDR=0x%08X / VALUE=0x%08X    ", k, endpoint->state.sco.source_buf[2]->read_ptr + k, endpoint->state.sco.source_buf[2]->read_ptr[k]);
             }
             printf("\n");
        }
        printf("\n");
        printf(" SAMPLES @ OUTPUT \n");
        printf(" SAMPLES [WORD16] delivered to OP (w/o SYNC_WORD) = %02d [WORD16] \n", NumOutSamples);
        printf("SIZE_INT(METADATA+PAYLOAD w/o SYNC)=%02d [WORD32]     SIZE_FRAC(METADATA+PAYLOAD w/o SYNC)=%02d [BYTES] \n", PayloadSizeAndMetadata_word32, PayloadSizeAndMetadata_FracWord32_bytes);
        cbuffer_advance_write_ptr(endpoint->state.sco.sink_buf[0], -NumOutSamples); //decr. out_buffer's write_ptr by NumOutSamples
        for (k=0; k<NumOutSamples; k++)
        {
               printf(" endpoint->state.sco.sink_buf[0]->write_ptr[%2d]:  ADDR=0x%08X / VALUE=0x%04X \n", k, endpoint->state.sco.sink_buf[0]->write_ptr, *(endpoint->state.sco.sink_buf[0]->write_ptr));
               cbuffer_advance_write_ptr(endpoint->state.sco.sink_buf[0], 1); //incr. out_buffer's write_ptr by 1 Sample
        }
        printf(" ep->state.sco.sink_buf[0]->size=0x%04X [BYTES] \t base_addr=0x%08X \t write_ptr=0x%08X \t read_ptr=0x%08X \n",
                 endpoint->state.sco.sink_buf[0]->size, endpoint->state.sco.sink_buf[0]->base_addr, endpoint->state.sco.sink_buf[0]->write_ptr, endpoint->state.sco.sink_buf[0]->read_ptr);
        counter_sco_src++;
#endif

        /**********************************/
        /* SCO EP SOURCE PROCESSING (END) */
        /**********************************/
    }
    else
    {
        /**********************************/
        /*    SCO EP SINK PROCESSING      */
        /**********************************/
        /*
         * Note concerning 'sco_to_air_length':
         * below comment was taken from sco_start function of 'stream_sco_common.c':
         *       The to-air frame length actually has two meanings according to HYDRA resp. BC. It is:
         *       1. The data frame so BT rate-matching doesn't throw away a portion
         *       of an encoded frame.
         *       2. The maximum length that audio will deliver data into the sco
         *       buffer. If the operator will run more than once per kick then it
         *       may produce more than a packet worth. BT needs to know otherwise
         *       it could discard data and then later will have insufficient to
         *       service a later packet
         *
         * to_air_length and from_air_length are both considered as timing_info->block_size in 'sco_common_get_timing()'
         *
         * Actually, unit of "to_air_length" is [16bit words] in Hydra and BC
         */

        /*
         *----------------------------------------------------------------------*
         * 1. READ WORDS from source buffer                                     *
         * 2. WRITE to TX BUFFER(s)                                             *
         * 3. APPEND COUNTER as metadata (After complete SCO packet was written *
         *----------------------------------------------------------------------*
         * NOTE concerning SCO PACKET SIZE:
         *  In pratice, we assume not to have odd number of BYTES per SCO packet in 64kbps on air rate, due to packet_size = 5*tesco for 64kbps, tesco even
         *
         * SCO_PACKET_SIZE is EVEN means: no remaining byte from a previous WORD16 to write (see note above on even number of Bytes)
         * Otherwise, BYTE of WORD16 as read in previous run would first to be written (saved in endpoint->state.sco.TxBytes2Write)
         * while (endpoint->state.sco.TxNumBytes2Write-- > 0)
         * {
         *     write(endpoint->state.sco.TxBytes2Write & 0xFF)
         *     endpoint->state.sco.TxBytes2Write>>8
         * }
         * endpoint->state.sco.TxNumBytes2Write = 0;   //no need to write BYTE of WORD16 from previous run
         *
         * Both variables 'endpoint->state.sco.TxBytes2Write' and 'endpoint->state.sco.TxNumBytes2Write' are obsolete in case SCO_PACKET_SIZE is EVEN
         */

        /*
         *---------------------------------------------------------------*
         * write [BYTES] to TX buffer:  write max. 1 sco packets at once *
         *                              to preserve double TX buffering  *
         *---------------------------------------------------------------*
         */

        /* get sco_packet_length in Bytes (payload only w/o counter metadata) */
        ScoPacketSize_Bytes = sco_to_air_length_get(stream_sco_get_hci_handle(endpoint))<<1; /* LENGTH stored in timing struct as amount of WORD16 */
        /* Amount of available Bytes that could be read from input buffer (Note: we assume an even number of Bytes: #BYTES=5*Tesco, Tesco even) --> NumBytes2Process is EVEN */
        unsigned int InData_AmountOfBytes = cbuffer_calc_amount_data_in_words(sco_state->source_buf[0])<<1;
        int          NumBytes2Process = min(InData_AmountOfBytes, ScoPacketSize_Bytes); /* newly available input data (WORD16) (max. SCO_PACKET_SIZE) */
        int          NumBytes2Process_word32 = NumBytes2Process>>2;                     /* Amount of complete word32 to process */
        int          NumBytes2Process_FracWord32_bytes = NumBytes2Process%4;            /* BYTES in remaining fract. word32 */
        int          idx = (sco_state->TxNumBytesWritten>>LOG2_ADDR_PER_WORD)+1;

        /* Toggle TX buffer index in case of 2 TX buffers (TX double buffering) */
        int         sel_BufIdx = (sco_state->sink_buf_num == 2) ? (sco_state->metadata_TxCnt%2) : 0;
#ifdef DEBUG_STREAM_SCO_NAPIER_SINK
        int         debug_buf_idx = sel_BufIdx; /* only for debug */
#endif


        /*
         *-----------------------------------------------------*
         * AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP                  *
         *-----------------------------------------------------*
         */
        if (sco_get_data_format(endpoint) == AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP)
        {
           /* loop on newly available input data (WORD16) (<= SCO_PACKET_SIZE) */
           for (k=0; k<NumBytes2Process_word32; k++)
           {   /* process complete word32 */
               cbuffer_read(sco_state->source_buf[0], &word16_2tuple[0], 2);  /* read next 2 WORD16 from input buffer */

               /* Write the 2 read Bytes to TX buffer: LSByte to MSByte */
               sco_state->sink_buf[sel_BufIdx]->write_ptr[idx++] = (((WBS_METADATA_TRANSFORM(word16_2tuple[1]))<<16) | (0xFFFF & (WBS_METADATA_TRANSFORM(word16_2tuple[0]))));
               sco_state->TxNumBytesWritten +=4;   /* increment counter that counts written Bytes of current TX SCO packet */

               /* check whether SCO TX packet is complete */
               if (sco_state->TxNumBytesWritten >= ScoPacketSize_Bytes)
               {   /*
                   * SCO_TX-packet complete:
                   * write metadata counter and apply TX buffer toggling
                   */
                   sco_state->metadata_TxCnt++;                /* increment counter and write it as in-band metadata */
                   sco_state->metadata_TxCnt &= 0x0000FFFF;    /* we have 16bit counter; upper 16 bit of 1st word are dummy(=0) */
                   sco_state->sink_buf[sel_BufIdx]->write_ptr[0] = sco_state->metadata_TxCnt; /* write metadata counter at last */
                   sel_BufIdx = (sco_state->metadata_TxCnt%2); /* toggle TX buffer to proceed writing to other TX buffer */
                   sco_state->TxNumBytesWritten = 0;           /* reset Byte counter */
                   idx = 1;                                             /* reset idx */
               }
           } /* end of for */

           /* process fractional word32 */
           if (NumBytes2Process_FracWord32_bytes > 0)
           {
                if (NumBytes2Process_FracWord32_bytes == 1)
                {
                     cbuffer_read(sco_state->source_buf[0], &word16_2tuple[0], 1);  /* read next WORD16 from input buffer */
                     sco_state->sink_buf[sel_BufIdx]->write_ptr[idx] = (0x00FF & (WBS_METADATA_TRANSFORM(word16_2tuple[0])));
                     sco_state->TxBytes2Write = word16_2tuple[0]>>8;  /* 1 Byte is written now and 1 Byte remains to write next time */
                     sco_state->TxNumBytes2Write = 1;
                     sco_state->TxNumBytesWritten +=1;                /* increment counter that counts written Bytes of current TX SCO packet */
                }
                else if (NumBytes2Process_FracWord32_bytes == 2)
                {
                     cbuffer_read(sco_state->source_buf[0], &word16_2tuple[0], 1);  /* read next WORD16 from input buffer */
                     sco_state->sink_buf[sel_BufIdx]->write_ptr[idx] = WBS_METADATA_TRANSFORM(word16_2tuple[0]);
                     sco_state->TxNumBytesWritten +=2;                /* increment counter that counts written Bytes of current TX SCO packet */
                }
                else if (NumBytes2Process_FracWord32_bytes == 3)
                {
                     cbuffer_read(sco_state->source_buf[0], &word16_2tuple[0], 2);  /* read next 2 WORD16 from input buffer */
                     sco_state->sink_buf[sel_BufIdx]->write_ptr[idx] = (((0x00FF & (WBS_METADATA_TRANSFORM(word16_2tuple[1])))<<16) | (0xFFFF & (WBS_METADATA_TRANSFORM(word16_2tuple[0]))));
                     sco_state->TxBytes2Write = word16_2tuple[1]>>8;  /* 3 Bytes are written now and 1 Byte remains to write next time */
                     sco_state->TxNumBytes2Write = 1;
                     sco_state->TxNumBytesWritten +=3;                /* increment counter that counts written Bytes of current TX SCO packet */
                }
                if (sco_state->TxNumBytesWritten >= ScoPacketSize_Bytes)
                {   /*
                     * SCO_TX-packet complete:
                     * write metadata counter and apply TX buffer toggling
                     */
                     sco_state->metadata_TxCnt++;                /* increment counter and write it as in-band metadata */
                     sco_state->metadata_TxCnt &= 0x0000FFFF;    /* we have 16bit counter; upper 16 bit of 1st word are dummy(=0) */
                     sco_state->sink_buf[sel_BufIdx]->write_ptr[0] = sco_state->metadata_TxCnt; /* write metadata counter at last */
                     sel_BufIdx = (sco_state->metadata_TxCnt%2); /* toggle TX buffer to proceed writing to other TX buffer */
                     sco_state->TxNumBytesWritten = 0;           /* reset Byte counter */
                     idx = 1;                                             /* reset idx */
                }
            }
        } /* end of AUDIO_DATA_FORMAT_16_BIT_BYTE_SWAP */
        else

        /*
         *-----------------------------------------------------*
         * AUDIO_DATA_FORMAT_16_BIT                            *
         *-----------------------------------------------------*
         */
        {
           /* loop on newly available input data (WORD16) (<= SCO_PACKET_SIZE) */
           for (k=0; k<NumBytes2Process_word32; k++)
           {   /* process complete word32 */
               cbuffer_read(sco_state->source_buf[0], &word16_2tuple[0], 2);  /* read next 2 WORD16 from input buffer */

               /* Write the 2 read Bytes to TX buffer: LSByte to MSByte */
               sco_state->sink_buf[sel_BufIdx]->write_ptr[idx++] = ((word16_2tuple[1])<<16 | (0xFFFF & word16_2tuple[0]));
               sco_state->TxNumBytesWritten +=4;   /* increment counter that counts written Bytes of current TX SCO packet */

               /* check whether SCO TX packet is complete */
               if (sco_state->TxNumBytesWritten >= ScoPacketSize_Bytes)
               {   /*
                   * SCO_TX-packet complete:
                   * write metadata counter and apply TX buffer toggling
                   */
                   sco_state->metadata_TxCnt++;                /* increment counter and write it as in-band metadata */
                   sco_state->metadata_TxCnt &= 0x0000FFFF;    /* we have 16bit counter; upper 16 bit of 1st word are dummy(=0) */
                   sco_state->sink_buf[sel_BufIdx]->write_ptr[0] = sco_state->metadata_TxCnt; /* write metadata counter at last */
                   sel_BufIdx = (sco_state->metadata_TxCnt%2); /* toggle TX buffer to proceed writing to other TX buffer */
                   sco_state->TxNumBytesWritten = 0;           /* reset Byte counter */
                   idx = 1;                                             /* reset idx */
               }
           } /* end of for */

           /* process fractional word32 */
           if (NumBytes2Process_FracWord32_bytes > 0)
           {
                if (NumBytes2Process_FracWord32_bytes == 1)
                {
                     cbuffer_read(sco_state->source_buf[0], &word16_2tuple[0], 1);  /* read next WORD16 from input buffer */
                     sco_state->sink_buf[sel_BufIdx]->write_ptr[idx] = (0x00FF & word16_2tuple[0]);
                     sco_state->TxBytes2Write = word16_2tuple[0]>>8;  /* 1 Byte is written now and 1 Byte remains to write next time */
                     sco_state->TxNumBytes2Write = 1;
                     sco_state->TxNumBytesWritten +=1;                /* increment counter that counts written Bytes of current TX SCO packet */
                }
                else if (NumBytes2Process_FracWord32_bytes == 2)
                {
                     cbuffer_read(sco_state->source_buf[0], &word16_2tuple[0], 1);  /* read next WORD16 from input buffer */
                     sco_state->sink_buf[sel_BufIdx]->write_ptr[idx] = word16_2tuple[0];
                     sco_state->TxNumBytesWritten +=2;                /* increment counter that counts written Bytes of current TX SCO packet */
                }
                else if (NumBytes2Process_FracWord32_bytes == 3)
                {
                     cbuffer_read(sco_state->source_buf[0], &word16_2tuple[0], 2);  /* read next 2 WORD16 from input buffer */
                     sco_state->sink_buf[sel_BufIdx]->write_ptr[idx] = ((0x00FF & (word16_2tuple[1]))<<16 | (0xFFFF & word16_2tuple[0]));
                     sco_state->TxBytes2Write = word16_2tuple[1]>>8;  /* 3 Bytes are written now and 1 Byte remains to write next time */
                     sco_state->TxNumBytes2Write = 1;
                     sco_state->TxNumBytesWritten +=3;                /* increment counter that counts written Bytes of current TX SCO packet */
                }
                if (sco_state->TxNumBytesWritten >= ScoPacketSize_Bytes)
                {   /*
                     * SCO_TX-packet complete:
                     * write metadata counter and apply TX buffer toggling
                     */
                     sco_state->metadata_TxCnt++;                /* increment counter and write it as in-band metadata */
                     sco_state->metadata_TxCnt &= 0x0000FFFF;    /* we have 16bit counter; upper 16 bit of 1st word are dummy(=0) */
                     sco_state->sink_buf[sel_BufIdx]->write_ptr[0] = sco_state->metadata_TxCnt; /* write metadata counter at last */
                     sel_BufIdx = (sco_state->metadata_TxCnt%2); /* toggle TX buffer to proceed writing to other TX buffer */
                     sco_state->TxNumBytesWritten = 0;           /* reset Byte counter */
                     idx = 1;                                             /* reset idx */
                }
            }
        } /* end of AUDIO_DATA_FORMAT_16_BIT */


#ifdef DEBUG_STREAM_SCO_NAPIER_SINK
        static unsigned counter_sco_sink;
        printf ("\n********************************************************\n");
        printf ("*** file:     components/stream/stream_sco_napier.c  ***\n");
        printf ("*** function: 'sco_kick'                             ***\n");
        printf ("*** SCO_SINK_EP gets KICKED.....                     ***\n");
        printf ("********************************************************\n");
        printf ("\n");
        printf(" AUDIO_DATA_FORMAT (connected OP)=%02d \n",  sco_get_data_format(endpoint));
        printf(" SCO PACKET SIZE (w/o  metadata) =%02d [BYTES] \n", ScoPacketSize_Bytes);
        printf("\n");
        printf ("******** TIMING *********\n");
        printf(" time_now = %04d [us] \t SCO_SINK_COUNTER = %02d \n", time_get_time(), counter_sco_sink);
        printf("\n");
        printf ("************ BUFFER INFO ************\n");
        printf(" TX BUFFER SIZE (=TO_AIR_BUFFER) (payload + counter) = %03d [WORD32] = %04d [BYTES] \n",
                 cbuffer_get_size_in_words(endpoint->state.sco.sink_buf[0]), cbuffer_get_size_in_addrs(endpoint->state.sco.sink_buf[0]));
        printf(" INPUT BUFFER SIZE (from OP)                         = %03d [WORD32] = %04d [BYTES] \n",
                 cbuffer_get_size_in_words(endpoint->state.sco.source_buf[0]), cbuffer_get_size_in_addrs(endpoint->state.sco.source_buf[0]));
        printf("\n");
        printf ("******** SAMPLES *********\n");
        printf(" SAMPLES @ INPUT \n");
        printf(" Num of Processed / Num total available input samples: %03d [BYTES] / %03d [BYTES] \n", min(InData_AmountOfBytes, ScoPacketSize_Bytes), InData_AmountOfBytes);
        cbuffer_advance_read_ptr(endpoint->state.sco.source_buf[0], -(min(InData_AmountOfBytes, ScoPacketSize_Bytes)>>1));
        for (k=0; k<(min(InData_AmountOfBytes, ScoPacketSize_Bytes)>>1); k++)
        {
               printf(" endpoint->state.sco.source_buf[0]->read_ptr[%2d]:  ADDR=0x%08X / VALUE=0x%04X \n", k, endpoint->state.sco.source_buf[0]->read_ptr, *(endpoint->state.sco.source_buf[0]->read_ptr));
               cbuffer_advance_read_ptr(endpoint->state.sco.source_buf[0], 1);
        }
        printf("\n");
        printf(" SAMPLES @ OUTPUT \n");
        for (k=0; k<cbuffer_get_size_in_words(endpoint->state.sco.sink_buf[0]); k++)
        {
             printf(" endpoint->state.sco.sink_buf[%d]->write_ptr[%02d]: ADDR=0x%08X / VALUE=0x%08X \n", debug_buf_idx, k, endpoint->state.sco.sink_buf[debug_buf_idx]->write_ptr+k, endpoint->state.sco.sink_buf[debug_buf_idx]->write_ptr[k]);
        }
        printf(" endpoint->state.sco.TxNumBytesWritten=%02d \t endpoint->state.sco.TxNumBytes2Write=%02d \t endpoint->state.sco.TxBytes2Write=0x%08X \n",
                 endpoint->state.sco.TxNumBytesWritten, endpoint->state.sco.TxNumBytes2Write, endpoint->state.sco.TxBytes2Write);
        counter_sco_sink++;
#endif

        /**********************************/
        /* SCO EP SINK PROCESSING (END)   */
        /**********************************/
    }
}


/*--------------------------------------------------------------------------*
 * get_metadata                                                             *
 *--------------------------------------------------------------------------*/
static sco_metadata_t get_metadata(int* rx_buffer_ptr)
{
    sco_metadata_t result;

    patch_fn_shared(stream_sco);

    /* The following metadata order is considered:
     *
     *  -------------------------- -------------------------- -------------------------- --------------------------
     * |    BYTE_3: MSB_STATUS    |    BYTE_2: LSB_STATUS    | BYTE_1: MSB_METADATA_LEN | BYTE_0: LSB_METADATA_LEN |
     *  --------------------------^--------------------------^--------------------------^--------------------------^
     *                                                                                                             |
     *                                                                                                       &rx_buffer_ptr[0]
     *
     *
     *  -------------------------- -------------------------- -------------------------- --------------------------
     * |    BYTE_3: MSB_COUNTER   |    BYTE_2: LSB_COUNTER   | BYTE_1: MSB_PAYLOAD_SIZE | BYTE_0: LSB_PAYLOAD_SIZE |
     *  --------------------------^--------------------------^--------------------------^--------------------------^
     *                                                                                                             |
     *                                                                                                       &rx_buffer_ptr[1]
     */

    /* metadata are 8 Bytes in SCO NAPIER*/
    result.metadata_len   =  rx_buffer_ptr[0] & 0x000000FF;   /* BYTE[0]: LSB of Lenght of in band metadata in BYTES */
    result.metadata_len  |=  rx_buffer_ptr[0] & 0x0000FF00;   /* BYTE[1]: MSB of Lenght of in band metadata in BYTES */

    result.status   = (rx_buffer_ptr[0]>>16)  & 0x000000FF;   /* BYTE[2]: LSB of Status of SCO packet (OK, CRC failed, Not received)  */
    result.status  |= (rx_buffer_ptr[0]>>16)  & 0x0000FF00;   /* BYTE[3]: MSB of Status of SCO packet (OK, CRC failed, Not received)  */

    result.payload_size   =  rx_buffer_ptr[1] & 0x000000FF;   /* BYTE[4]: LSB of payload size in BYTES */
    result.payload_size  |=  rx_buffer_ptr[1] & 0x0000FF00;   /* BYTE[5]: MSB of payload size in BYTES */

    result.counter   = (rx_buffer_ptr[1]>>16) & 0x000000FF;   /* BYTE[6]: MSB of counter */
    result.counter  |= (rx_buffer_ptr[1]>>16) & 0x0000FF00;   /* BYTE[7]: LSB of counter */

    return result;
}


/*--------------------------------------------------------------------------*
 * sco_ErrDataProcess_getRxBufIdx                                                 *
 *--------------------------------------------------------------------------*/
static sco_majority_vote_t sco_ErrDataProcess_MajorityVoting(sco_metadata_t* metadata_rx, int32 buf_num, tCbuffer** rx_buf_ptr, bool voting_is_bypassed)
{
    unsigned              hamingWeight_RxBuf_1_0;
    unsigned              hamingWeight_RxBuf_2_0;
    unsigned              hamingWeight_RxBuf_2_1;
    sco_majority_vote_t   result;
    int k;

    patch_fn_shared(stream_sco);

    /* check crc result of all rx buffers: if CRC of one out of these data is OK select it -> return buf idx (0/1/2) */
    for (k=0; k<buf_num; k++)
    {
         if (metadata_rx[k].status == RX_BUFFER_STATUS_VALID)
         {
             result.bufIdx = k;          /* selected Buffer Index: return idx=k (just selecting buffer[k])   */
             result.isVoting = 0;        /* indicates whetehr majority voting should be applied(1) or not(0) */
             result.quality_metric = 0;  /* there is one valiud packet, so we assume best quality */
             return (result);
         }
    }

    /*
     * all received sco packets have either RX_BUFFER_STATUS_BAD_CRC or RX_BUFFER_STATUS_NONE
     * Check whether we apply majority voting in this case
     */
    if (voting_is_bypassed == TRUE)
    {   /* MAJORITY VOTING BYPASSED */
        result.isVoting = FALSE;         /* voting will not be applied */
        result.bufIdx = 0;               /* RX_buf[0] selected */
        result.quality_metric = 0;       /* number of questionable bits per sco packet */
        return (result);
    }

   /*
    * Majority Voting is applied: for majority voting we do some analysis subsequent
    */
    if (buf_num == 1)
    {
               result.bufIdx = 0;
               result.isVoting = 0;
               result.quality_metric = metadata_rx[0].payload_size; /* we tread all payload bits of SCO packets as uncertain */
               return (result);
    }
    else if (buf_num == 2)
    {   /* 2 RX buffers, both with CRC fails*/
        if (  (metadata_rx[0].status == RX_BUFFER_STATUS_BAD_CRC)
           && (metadata_rx[1].status == RX_BUFFER_STATUS_BAD_CRC))
        {
               result.bufIdx = 0;
               result.isVoting = 0;
               hamingWeight_RxBuf_1_0 = calc_HammingWeight(metadata_rx[0].payload_size, &rx_buf_ptr[0]->read_ptr[2], &rx_buf_ptr[1]->read_ptr[2]); /* corelate payload: ignore first 2 word32 -> metadata */
               result.quality_metric = hamingWeight_RxBuf_1_0;
               return (result);
        }
        else
        {
               /* 2 RX buffers, one with RX_BUFFER_STATUS_BAD_CRC, the other with RX_BUFFER_STATUS_NONE -> select the one out of the two with RX_BUFFER_STATUS_BAD_CRC */
               for (k=0; k<2; k++)
               {
                  if (metadata_rx[k].status == RX_BUFFER_STATUS_BAD_CRC)
                  {
                      result.bufIdx = k;    /* selected Buffer Index: return idx=k (just selecting buffer[k])   */
                      result.isVoting = 0;  /* indicates whether majority voting should be applied(1) or not(0) */
                      result.quality_metric = metadata_rx[0].payload_size; /* we tread all payload bits of SCO packets as uncertain */
                      return (result);
                  }
               }

               /* 2 RX buffers, both RX_BUFFER_STATUS_NONE */
               result.bufIdx = 0;
               result.isVoting = 0;
               result.quality_metric = metadata_rx[0].payload_size; /* we tread all payload bits of SCO packets as uncertain */
               return (result);
        }
    }
    else
    {   /* 3 RX buffers */
        if (  (metadata_rx[0].status == RX_BUFFER_STATUS_BAD_CRC)
           && (metadata_rx[1].status == RX_BUFFER_STATUS_BAD_CRC)
           && (metadata_rx[2].status == RX_BUFFER_STATUS_BAD_CRC))
        {
            /*
             **************************************
             * 3 RX buffers -> All have CRC error *
             **************************************
             */
             // check wheter 2 out of the 3 have improved correlation on the poayload -> select one out of the two
             // otherwise do xor pairwise on all three buffers (use metadata from first buffer)
             hamingWeight_RxBuf_1_0 = calc_HammingWeight(metadata_rx[0].payload_size, &rx_buf_ptr[0]->read_ptr[2], &rx_buf_ptr[1]->read_ptr[2]); /* corelate payload: ignore first 2 word32 -> metadata */
             hamingWeight_RxBuf_2_0 = calc_HammingWeight(metadata_rx[0].payload_size, &rx_buf_ptr[0]->read_ptr[2], &rx_buf_ptr[2]->read_ptr[2]); /* corelate payload: ignore first 2 word32 -> metadata */
             hamingWeight_RxBuf_2_1 = calc_HammingWeight(metadata_rx[1].payload_size, &rx_buf_ptr[1]->read_ptr[2], &rx_buf_ptr[2]->read_ptr[2]); /* corelate payload: ignore first 2 word32 -> metadata */

             if (     (hamingWeight_RxBuf_1_0 < (hamingWeight_RxBuf_2_0>>1)) && (hamingWeight_RxBuf_1_0 < (hamingWeight_RxBuf_2_1>>1)))
             {   /* RX_buf_2 poorly correlated -> discard */
                 result.bufIdx = 0;    /* selected Buffer Index: select RX_buf_0 or RX_buf_1 */
                 result.isVoting = 0;  /* indicates whetehr majority voting should be applied(1) or not(0) */
                 /* quality metric counts uncertain bits per SCO packet; the lower the value the better the quality */
                 result.quality_metric = hamingWeight_RxBuf_1_0;
             }
             else if ((hamingWeight_RxBuf_2_0 < (hamingWeight_RxBuf_1_0>>1)) && (hamingWeight_RxBuf_2_0 < (hamingWeight_RxBuf_2_1>>1)))
             {   /* RX_buf_1 poorly correlated -> discard */
                 result.bufIdx = 0;    /* selected Buffer Index: select RX_buf_0 or RX_buf_2 */
                 result.isVoting = 0;  /* indicates whetehr majority voting should be applied(1) or not(0) */
                 /* quality metric counts uncertain bits per SCO packet; the lower the value the better the quality */
                 result.quality_metric = hamingWeight_RxBuf_2_0;
             }
             else if ((hamingWeight_RxBuf_2_1 < (hamingWeight_RxBuf_1_0>>1)) && (hamingWeight_RxBuf_2_1 < (hamingWeight_RxBuf_2_0>>1)))
             {   /* RX_buf_0 poorly correlated -> discard */
                 result.bufIdx = 1;    /* selected Buffer Index: select RX_buf_1 or RX_buf_2 */
                 result.isVoting = 0;  /* indicates whetehr majority voting should be applied(1) or not(0) */
                /* quality metric counts uncertain bits per SCO packet; the lower the value the better the quality */
                result.quality_metric = hamingWeight_RxBuf_2_1;
             }
             else
             { /* all RX buffers have about the same correlation -> 2 out of 3 voting */
                 result.bufIdx = 0;     /* selected Buffer Index: select RX_buf_1 or RX_buf_2 */
                 result.isVoting = 1;   /* indicates whetehr majority voting should be applied(1) or not(0): process majority voting (best 2 out of 3) */
                 /* quality metric counts uncertain bits per SCO packet; the lower the value the better the quality */
                 result.quality_metric = calc_QualityMetric_3RxBuffers(metadata_rx[0].payload_size, &rx_buf_ptr[0]->read_ptr[2], &rx_buf_ptr[1]->read_ptr[2], &rx_buf_ptr[2]->read_ptr[2]);
             }
             return (result);
        }
        else
        {
             /*
              *****************************************
              * 3 RX buffers -> Max. 2 have CRC error *
              * the others have RX_BUFFER_STATUS_NONE *
              *****************************************
              */
              if (  (metadata_rx[0].status == RX_BUFFER_STATUS_BAD_CRC)
                 && (metadata_rx[1].status == RX_BUFFER_STATUS_BAD_CRC))
              {
                  result.bufIdx = 0;
                  result.isVoting = 0;
                  result.quality_metric = calc_HammingWeight(metadata_rx[0].payload_size, &rx_buf_ptr[0]->read_ptr[2], &rx_buf_ptr[1]->read_ptr[2]); /* corelate payload: ignore first 2 word32 -> metadata */
                  return (result);
              }
              else if (  (metadata_rx[0].status == RX_BUFFER_STATUS_BAD_CRC)
                      && (metadata_rx[2].status == RX_BUFFER_STATUS_BAD_CRC))
              {
                  result.bufIdx = 0;
                  result.isVoting = 0;
                  result.quality_metric = calc_HammingWeight(metadata_rx[0].payload_size, &rx_buf_ptr[0]->read_ptr[2], &rx_buf_ptr[2]->read_ptr[2]); /* corelate payload: ignore first 2 word32 -> metadata */
                  return (result);
              }
              else if (  (metadata_rx[1].status == RX_BUFFER_STATUS_BAD_CRC)
                      && (metadata_rx[2].status == RX_BUFFER_STATUS_BAD_CRC))
              {
                  result.bufIdx = 1;
                  result.isVoting = 0;
                  result.quality_metric = calc_HammingWeight(metadata_rx[1].payload_size, &rx_buf_ptr[1]->read_ptr[2], &rx_buf_ptr[2]->read_ptr[2]); /* corelate payload: ignore first 2 word32 -> metadata */
                  return (result);
              }
              else
              {
                     /* 3 RX buffers, one with RX_BUFFER_STATUS_BAD_CRC, the others with RX_BUFFER_STATUS_NONE -> select the one out of the two with RX_BUFFER_STATUS_BAD_CRC */
                     for (k=0; k<3; k++)
                     {
                        if (metadata_rx[k].status == RX_BUFFER_STATUS_BAD_CRC)
                        {
                            result.bufIdx = k;                                   /* selected Buffer Index: return idx=k (just selecting buffer[k])   */
                            result.isVoting = 0;                                 /* indicates whether majority voting should be applied(1) or not(0) */
                            result.quality_metric = metadata_rx[0].payload_size; /* we tread all payload bits of SCO packets as uncertain */
                            return (result);
                        }
                     }
                     /* 3 RX buffers, all 3 RX_BUFFER_STATUS_NONE */
                     result.bufIdx = 0;
                     result.isVoting = 0;
                     result.quality_metric = metadata_rx[0].payload_size; /* we tread all payload bits of SCO packets as uncertain */
                     return (result);
              }
        }
    }
}


/*--------------------------------------------------------------------------*
 * calc_HammingWeight                                                       *
 *--------------------------------------------------------------------------*/
static unsigned calc_HammingWeight(int num_Bytes, int* rx_buf_ptr0, int* rx_buf_ptr1)
{
    int k;
    /* int j; */
    unsigned right_shift;
    unsigned byte_compare;
    unsigned HammingWeight = 0;

    patch_fn_shared(stream_sco);

    for (k=0; k<num_Bytes; k++)
    {
          /* bitwise xor of Bytes */
          right_shift = 8*(k%4); /* 0, 8, 16, 24 */
          byte_compare = (((rx_buf_ptr0[k>>2])>>right_shift) ^ ((rx_buf_ptr1[k>>2])>>right_shift)) & 0xFF;     /* Note: rx_buf_ptr0, rx_buf_ptr0 point to WORD32 */

          /*-------------------------*/
          /* count '1' bits in current BYTE */
          /* Algorithm_1: probably more efficient than Algorithm_2 */
          while(byte_compare)
          {
              HammingWeight++;
              byte_compare &= (byte_compare- 1);
          }

          /*-------------------------*/
          /* Algorithm_2 */
          /*
          for (j=0; j<8; j++)
          {
              if ((byte_compare>>j) & 0x1)
                   HammingWeight++;
          }
          */
    }
    return (HammingWeight);
}


/*--------------------------------------------------------------------------*
 * calc_QualityMetric_3RxBuffers                                                       *
 *--------------------------------------------------------------------------*/
static unsigned calc_QualityMetric_3RxBuffers(int num_Bytes, int* rx_buf_ptr0, int* rx_buf_ptr1, int* rx_buf_ptr2)
{
    int k;
    unsigned right_shift;
    unsigned byte_compare;
    unsigned QualityWeight = 0;

    patch_fn_shared(stream_sco);

    /* if all 3 bits are equal --> 0, otherwise -> 1 */
    /* the higher the metric value, the worse the quality */
    for (k=0; k<num_Bytes; k++)
    {
          right_shift = 8*(k%4); /* 0, 8, 16, 24 */
          byte_compare = (((rx_buf_ptr0[k>>2])>>right_shift) | ((rx_buf_ptr1[k>>2])>>right_shift) | ((rx_buf_ptr2[k>>2])>>right_shift)) & 0xFF;     /* Note: rx_buf_ptr points to WORD32 */
          byte_compare &= (~(((rx_buf_ptr0[k>>2])>>right_shift) & ((rx_buf_ptr1[k>>2])>>right_shift) & ((rx_buf_ptr2[k>>2])>>right_shift))) & 0xFF;

          /* count '1' bits in current BYTE */
          while(byte_compare)
          {
              QualityWeight++;
              byte_compare &= (byte_compare- 1);
          }
    }
    return (QualityWeight);
}


/*--------------------------------------------------------------------------*
 * sco_configure_napier                                                     *
 *--------------------------------------------------------------------------*/
static bool sco_configure_napier(ENDPOINT *endpoint, unsigned int key, uint32 value)
{
    patch_fn_shared(stream_sco);

    switch(key)
    {
        case STREAM_CONFIG_KEY_STREAM_SCO_SRC_MAJORITY_VOTE_BYPASS:
             if(endpoint->direction == SOURCE)
             {   /* majority voting is only applied on RX buffers as read by SCO_SOURCE */
                 endpoint->state.sco.majority_vote_bypass = (value==1) ? TRUE : FALSE;
                 return TRUE;
             }
             else
             {   /* indicates, that majority voting is not relevant for SCO_SINK */
                 return FALSE;
             }
             break;

        case STREAM_CONFIG_KEY_STREAM_SCO_SRC_MAJORITY_VOTE_QUESTIONABLE_BITS_MAX:
             if(endpoint->direction == SOURCE)
             {   /* majority voting: max. number of uncorrelated bits to status overwrite in case of CRC_FAIL */
                 endpoint->state.sco.majority_vote_questionable_bits_max = (value<256) ? value : 255; /* uint8 */
                 return TRUE;
             }
             else
             {   /* indicates, that majority voting is not relevant for SCO_SINK */
                 return FALSE;
             }
             break;

        default:
             /* call common function */
             return (sco_configure(endpoint, key, value));
    }
}


/*--------------------------------------------------------------------------*
 * sco_get_config_napier                                                    *
 *--------------------------------------------------------------------------*/
static bool sco_get_config_napier(ENDPOINT *endpoint, unsigned int key, ENDPOINT_GET_CONFIG_RESULT* result)
{
    patch_fn_shared(stream_sco);

    switch(key)
    {
        case STREAM_CONFIG_KEY_STREAM_SCO_SRC_MAJORITY_VOTE_BYPASS:
             if(endpoint->direction == SOURCE)
             {   /* majority voting is only applied on RX buffers as read by SCO_SOURCE */
                 result->u.value = (uint32) endpoint->state.sco.majority_vote_bypass;
                 return TRUE;
             }
             else
             {   /* indicates, that majority voting is not relevant for SCO_SINK */
                 return FALSE;
             }
             break;

        case STREAM_CONFIG_KEY_STREAM_SCO_SRC_MAJORITY_VOTE_QUESTIONABLE_BITS_MAX:
             if(endpoint->direction == SOURCE)
             {   /* majority voting: max. number of uncorrelated bits to status overwrite in case of CRC_FAIL */
                 result->u.value = (uint32) endpoint->state.sco.majority_vote_questionable_bits_max;
                 return TRUE;
             }
             else
             {   /* indicates, that majority voting is not relevant for SCO_SINK */
                 return FALSE;
             }
             break;

        default:
             /* call common function */
             return (sco_get_config(endpoint, key, result));
    }
}


/*--------------------------------------------------------------------------*
 * flush_endpoint_buffers                                                   *
 *--------------------------------------------------------------------------*/
void flush_endpoint_buffers(ENDPOINT *ep)
{
    tCbuffer* cbuffer = stream_transform_from_endpoint(ep)->buffer;

    patch_fn_shared(stream_sco);

    if(ep->direction == SOURCE)
    {
        cbuffer_empty_buffer(cbuffer);
    }
    else
    {
        cbuffer_flush_and_fill(cbuffer, 0);
    }
}





