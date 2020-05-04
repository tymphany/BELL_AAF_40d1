/****************************************************************************
 * Copyright (c) 2015 Qualcomm Technologies International, Ltd.
 * All Rights Reserved.
 * Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 * Notifications and licenses are retained for attribution purposes only
****************************************************************************/
/**
 * \ingroup endpoints
 * \file  stream_a2dp_napier.c
 *
 * stream a2dp type file. <br>
 * This file contains stream functions for Napier a2dp endpoints. <br>
 *
 */

/****************************************************************************
Include Files
*/

#include "stream_private.h"
#include "stream_endpoint_a2dp.h"
#include "buffer.h"
#include "pl_assert.h"
#include "pmalloc/pmalloc.h"
#include "napier_modules/bulk_data_interface/bulk_data_interface.h"

/****************************************************************************
Private Type Declarations
*/

typedef struct {
    union {
        uint32 reg32;
        struct {
            uint32 CSRC_count     : 4;  /* Contains the number of CSRC identifiers (defined below) that follow the fixed header */
            uint32 extension      : 1;  /*Indicates presence of an Extension header between standard header and payload */
                                        /* data. This is application or profile specific*/
            uint32 padding        : 1;  /* Used to indicate if there are extra padding bytes at the end*/
                                        /* of the RTP packet. A padding might be used to fill up a block*/
                                        /* of certain size, for example as required by an encryption algorithm.*/
                                        /* The last byte of the padding contains the number of padding bytes that were added (including itself)*/
            uint32 version        : 2;  /* Indicates the version of the protocol. Current version is 2*/


            uint32 marker         : 1;  /* Used at the application level and defined by a profile. If it is set, it means that the current data has some special relevance for the application.*/
            uint32 payloadType    : 7;  /* Indicates the format of the payload and determines its interpretation by the application. This is specified by an RTP profile*/
            uint32 sequenceNumber : 16; /* The sequence number is incremented by one for each RTP data packet sent and is to be used by the*/
                                        /* receiver to detect packet loss and to restore packet sequence*/
        } bits;
    } header1;
    uint32 timeStamp;                   /* Used to enable the receiver to play back the received samples at appropriate intervals*/
    uint32 SSRC_identifier;             /* Synchronization source identifier uniquely identifies the source of a stream*/
}sRtpHeader_t;

/****************************************************************************
Private Constant Declarations
*/


/****************************************************************************
Private Macro Declarations
*/
/** Number of parameters to be received when the endpoint is created */
#define A2DP_PARAMS_NUM  2
/** The size of the mmu buffer that holds incoming a2dp data (usable octets) */
#define A2DP_MMU_DATA_BUFFER_SIZE_BYTES (2048)
/** The size of the mmu buffer that holds incoming a2dp metadata (usable octets) */
#define A2DP_META_BUFFER_SIZE_BYTES (64)
/** The number of usec to wait before attempting to retry pushing data forwards */
#define A2DP_SELF_KICK_THRESHOLD 2000

#ifndef INSTALL_METADATA
#error "METADATA must be installed for A2DP Endpoint on Napier"
#endif


#if defined(A2DP_DEBUG_LOG)
#define A2DP_DBG_MSG(x)                 L3_DBG_MSG(x)
#define A2DP_DBG_MSG1(x, a)             L3_DBG_MSG1(x, a)
#define A2DP_DBG_MSG2(x, a, b)          L3_DBG_MSG2(x, a, b)
#define A2DP_DBG_MSG3(x, a, b, c)       L3_DBG_MSG3(x, a, b, c)
#define A2DP_DBG_MSG4(x, a, b, c, d)    L3_DBG_MSG4(x, a, b, c, d)
#define A2DP_DBG_MSG5(x, a, b, c, d, e) L3_DBG_MSG5(x, a, b, c, d, e)
#else  /* A2DP_DEBUG_LOG */
#define A2DP_DBG_MSG(x)                 ((void)0)
#define A2DP_DBG_MSG1(x, a)             ((void)0)
#define A2DP_DBG_MSG2(x, a, b)          ((void)0)
#define A2DP_DBG_MSG3(x, a, b, c)       ((void)0)
#define A2DP_DBG_MSG4(x, a, b, c, d)    ((void)0)
#define A2DP_DBG_MSG5(x, a, b, c, d, e) ((void)0)
#endif /* A2DP_DEBUG_LOG */

#if defined(A2DP_DEBUG_LOG) ||  defined(A2DP_DEBUG_ERROR_LOG)
#define A2DP_DBG_ERRORMSG(x)                 L0_DBG_MSG(x)
#define A2DP_DBG_ERRORMSG1(x, a)             L0_DBG_MSG1(x, a)
#define A2DP_DBG_ERRORMSG2(x, a, b)          L0_DBG_MSG2(x, a, b)
#define A2DP_DBG_ERRORMSG3(x, a, b, c)       L0_DBG_MSG3(x, a, b, c)
#define A2DP_DBG_ERRORMSG4(x, a, b, c, d)    L0_DBG_MSG4(x, a, b, c, d)
#define A2DP_DBG_ERRORMSG5(x, a, b, c, d, e) L0_DBG_MSG5(x, a, b, c, d, e)
#else  /* A2DP_DEBUG_LOG || A2DP_DEBUG_ERROR_LOG */
#define A2DP_DBG_ERRORMSG(x)                 ((void)0)
#define A2DP_DBG_ERRORMSG1(x, a)             ((void)0)
#define A2DP_DBG_ERRORMSG2(x, a, b)          ((void)0)
#define A2DP_DBG_ERRORMSG3(x, a, b, c)       ((void)0)
#define A2DP_DBG_ERRORMSG4(x, a, b, c, d)    ((void)0)
#define A2DP_DBG_ERRORMSG5(x, a, b, c, d, e) ((void)0)
#endif /* A2DP_DEBUG_LOG || A2DP_DEBUG_ERROR_LOG */


/* here are some enhanced debugging features used during development. Once the code is stable, this may be removed */

/* print some low-level debug information in Kalsim*/
// #define DEBUG_PRINT_ENCODED_FRAME    // print out all encoded data, usefull for Kalism debugging
//#define DEBUG_AHB_BRIDGE              // debug the (potentially faulty) AHB bridge
#define ADVANCED_DEBUG                  // output some more debug messages
//#define DEBUG_LOG_BUFFER              // allocate a large buffer to dump inormation into. Used for debugging the M0 - Kalimba interface
#define FILL_PADDING_BYTES              // fill padding bytes with 01, 02, 03 etc. instead of just leaving the existing bytes in the buffer. For debugging only


#ifdef DEBUG_PRINT_ENCODED_FRAME
#include "stdio.h"
#endif

/* fill padding bytes with byte count for debugging */
#ifdef DEBUG_LOG_BUFFER
#define DBG_MAX_SIZE 4096
uint16 *a2dpdbg = NULL;
int dbgcnt = 0;
int dbgcntAll = 0;
#endif


/****************************************************************************
Private Variable Definitions
*/
/** The number of usec without data arriving over the air before we consider
 *  the input stream to be stalled.
 */
static volatile
#ifdef __KCC__
_Pragma("datasection DM_SHARED")
#endif /* __KCC__ */
unsigned a2dp_stall_threshold = 80000;

/****************************************************************************
Private Function Declarations
*/
static void a2dp_get_timing(ENDPOINT *, ENDPOINT_TIMING_INFORMATION *);
static bool a2dp_close(ENDPOINT *endpoint, bool *pending);
static inline void a2dp_kick(ENDPOINT *endpoint, ENDPOINT_KICK_DIRECTION kick_dir);
static bool a2dp_start(ENDPOINT *, KICK_OBJECT *);
static bool a2dp_stop(ENDPOINT *);
static void a2dp_stall_handler(void *ep);
static void stream_a2dp_post_create_callback(unsigned sid, uint16 status);
static void stream_a2dp_post_close_callback(unsigned sid, uint16 status);
static void stream_a2dp_service_routine(void* kick_object);
static bool a2dp_configure_napier(ENDPOINT *endpoint, unsigned key, uint32 value);
static bool a2dp_connect_napier(ENDPOINT *endpoint, tCbuffer *cbuffer_ptr, ENDPOINT *ep_to_kick, bool* start_on_connect);
static void delete_consumed_metadata_tag(tCbuffer *cbuff, unsigned int  octets_consumed);


DEFINE_ENDPOINT_FUNCTIONS(a2dp_functions, a2dp_close, a2dp_connect_napier,
                          a2dp_disconnect, a2dp_buffer_details,
                          a2dp_kick, stream_sched_kick_dummy, a2dp_start,
                          a2dp_stop, a2dp_configure_napier, a2dp_get_config,
                          a2dp_get_timing, stream_sync_sids_dummy,
                          stream_have_same_clock_common);

/****************************************************************************
Global Variables
*/


/****************************************************************************
Public Function Definitions
*/


/****************************************************************************
 *
 * stream_a2dp_get_endpoint
 *
 */
ENDPOINT *stream_a2dp_get_endpoint(unsigned con_id, ENDPOINT_DIRECTION dir,
                                    unsigned num_params, unsigned *params, bool *pending)
{
    unsigned key;
    patch_fn_shared(stream_a2dp);

    if (num_params != A2DP_PARAMS_NUM)
    {
        return NULL;
    }

    endpoint_a2dp_state *a2dp;

    /*
     * Make unique key for A2DP endpoint from ACL handle and CID
     * So we can identify if a endpoint with these parameters already
     * exists.
     * This works on Napier since key is 32bit and each param has 16
     * valid bits only.
     * */

    key = (params[0] & 0xFFFF) << 16;
    key |= (params[1] & 0xFFFF);

    ENDPOINT *endpoint = stream_get_endpoint_from_key_and_functions(key, dir,
                                                  &endpoint_a2dp_functions);

    /* if it already exists, return it, else start creating a new one */
    if (endpoint != NULL)
    {
        return endpoint;
    }

    /* The DSP does not need to store this values, but it needs to pass them to bulkIf/comms*/
    uint16 optional_params[3];

    if ((endpoint = STREAM_NEW_ENDPOINT(a2dp, key, dir, con_id)) == NULL)
    {
        return NULL;
    }
    optional_params[0] = params[0];
    optional_params[1] = params[1];
    optional_params[2] = stream_external_id_from_endpoint(endpoint);
    a2dp = &endpoint->state.a2dp;

    /* Initialise endpoint default values */
    a2dp->running = FALSE;
    endpoint->can_be_closed = TRUE;
    endpoint->can_be_destroyed = FALSE;
    endpoint->is_real = TRUE;
    *pending = TRUE;

    /* TODO_NAPIER: call bulk IF to create the bulk endpoint and send Open req*/
    sBdif_Flags_t flags;
    flags.allFlags = 0;
    if(SOURCE == endpoint->direction)
    {
        flags.bits.bufferOwner = BULKIF_KALIMBA;
        a2dp->source_buf = cbuffer_create(0, A2DP_META_BUFFER_SIZE_BYTES, BUF_DESC_SW_BUFFER);
        BUFF_METADATA_SET(a2dp->source_buf);
        buff_metadata_enable(a2dp->source_buf);

        a2dp->bulk_data_if = (void*)bulk_data_interface_new(0, flags, BULKIF_TYPE_L2CAP_FROM_AIR,
                                                                A2DP_MMU_DATA_BUFFER_SIZE_BYTES,
                                                                A2DP_META_BUFFER_SIZE_BYTES,
                                                                optional_params,
                                                                stream_a2dp_post_create_callback);
    }
    else
    {
        flags.bits.bufferOwner = BULKIF_ARM;
        a2dp->sink_buf = cbuffer_create(0, A2DP_META_BUFFER_SIZE_BYTES, BUF_DESC_SW_BUFFER);
        BUFF_METADATA_SET(a2dp->sink_buf);
        buff_metadata_enable(a2dp->sink_buf);

        a2dp->bulk_data_if = (void*)bulk_data_interface_new(0, flags, BULKIF_TYPE_L2CAP_TO_AIR,
                                                                0,
                                                                0,
                                                                optional_params,
                                                                stream_a2dp_post_create_callback);
    }

    /* Check if the bulk_data_if has been correctly created. */
    if (a2dp->bulk_data_if == NULL)
    {
        if (SOURCE == endpoint->direction)
        {
            cbuffer_destroy(endpoint->state.a2dp.source_buf);
        }
        else
        {
            cbuffer_destroy(endpoint->state.a2dp.sink_buf);
        }
        stream_destroy_endpoint(endpoint);
        return NULL;
    }

    return endpoint;
}


/****************************************************************************
 *
 * stream_a2dp_close_endpoint
 * \param ep pointer to the endpoint that is being closed
 */
static bool a2dp_close(ENDPOINT *endpoint, bool *pending)
{
    sBdif_Interface_t* bulk_data_if = endpoint->state.a2dp.bulk_data_if;
    bulk_data_interface_remove(bulk_data_if->bulkifId,
                                    stream_external_id_from_endpoint(endpoint),
                                    stream_a2dp_post_close_callback);
    *pending = TRUE;
    return TRUE;
}


static void a2dp_get_timing(ENDPOINT *ep, ENDPOINT_TIMING_INFORMATION *time_info)
{
    time_info->period = 0;
    /** TODO: check if kicks are required for an a2dp sink  */
    if (ep->direction == SINK) {
        time_info->wants_kicks = TRUE;
    }
    else {
        time_info->wants_kicks = FALSE;
    }
    a2dp_get_timing_common(ep, time_info);
}


#define FRAG_LEN_MASK (0x7FFF)
#define PKT_BOUNDARY_SHIFT (15)
#define PKT_BOUNDARY_MASK (1 << PKT_BOUNDARY_SHIFT)
#define EOF_MARKER (0xFFFF)

static void a2dp_append_metadata_to_buf(tCbuffer *buf, metadata_tag *tag, uint16 length, bool eof)
{
    bool added;

    METADATA_TIME_OF_ARRIVAL_SET(tag, time_get_time());

    tag->length = length;
    tag->length &= FRAG_LEN_MASK;

    interrupt_block();

    if (eof)
    {
        tag->length = 0;
    }
    else
    {
        METADATA_PACKET_START_SET(tag);
        if (tag->length & PKT_BOUNDARY_MASK)
        {
            METADATA_PACKET_END_SET(tag);
        }
        METADATA_STREAM_END_UNSET(tag);
    }
    added= buff_metadata_append(buf, tag, 0, tag->length);
    PL_ASSERT(added);
    interrupt_unblock();
}



/** signal that data has been drained  */
static void a2dp_eof_callback(unsigned data)
{
    uint16 *bulkifId = (uint16 *)(uintptr_t)data;
    bulk_data_interface_signal_data_drained(*bulkifId);
}


static void a2dp_self_kick(void *priv)
{
    ((ENDPOINT*)(priv))->state.a2dp.self_kick_timer_id = TIMER_ID_INVALID;
    a2dp_kick((ENDPOINT*)(priv), STREAM_KICK_INTERNAL);
}






/**
 * \brief kick the a2dp ep
 * \param ep pointer to the endpoint that is being kicked
 * \param kick_dir direction of the kick
 */
static inline void a2dp_kick(ENDPOINT *ep, ENDPOINT_KICK_DIRECTION kick_dir)
{
    endpoint_a2dp_state *a2dp           = &ep->state.a2dp;
    sBdif_Interface_t* bulk_data_if     = (sBdif_Interface_t*)(ep->state.a2dp.bulk_data_if);
    sBdif_Info_t* bulk_data_ptr         = bulk_data_if->ptBdifInfoData;
    sBdif_Info_t* bulk_meta_data_ptr    = bulk_data_if->ptBdifInfoMetaData;
    bool selfKick = FALSE;
    uint16 a2dp_packet_size = 0;

    patch_fn_shared(stream_a2dp_kick);

    /* A2DPSource */
    if (SOURCE == ep->direction)
    {
        metadata_tag *sinkBufMetaTag=NULL;
        unsigned int available_bytes_in_sinkbuf = 0;

        /* if we have a an operator connected do further processing */
        if (ep->connected_to)
        {
            /* How much is the max number of bytes we can copy into sink buffer ? */
            available_bytes_in_sinkbuf = cbuffer_get_size_ex(a2dp->sink_buf);
            //data_available_from_src = bulk_data_interface_get_fillLevelBytes(bulk_data_ptr);

#ifdef INSTALL_METADATA
            if (BUFF_METADATA(a2dp->sink_buf))
            {
                /* Limit to space available according to sink buffer metadata */
                available_bytes_in_sinkbuf = buff_metadata_available_space(a2dp->sink_buf);
            }
#endif

            /* we are only getting complete L2CAP packets from M0, so check how many we've got
             * Each packet creates a 2 Byte metadata entry. If we want to know, how many packets
             * are in the buffer we can read the metadata buffer fill level and divide it by 2  */
            uint16 numRemainingA2dpPkts = bulk_data_interface_get_fillLevelBytes(bulk_meta_data_ptr) >> 1;

            do {


#ifdef INSTALL_METADATA

                /* check if we have space for more metadata in the queue */

                if (buff_metadata_tag_threshold_exceeded())
                {
                    selfKick = TRUE;
                    break;
                }

                /* check if we have metadata space available in the sink buffer */

                /* generate a new tag. If that fails, we do not have memory available, so
                 * we will just wait until more tags have been consumed. A fault is reported
                 * within the call so we can see what happens later. Don't think this should
                 * end in panic here.
                 */
                sinkBufMetaTag = buff_metadata_new_tag();
                if (sinkBufMetaTag == NULL)    {

                    selfKick = TRUE;
                    break;
                }
#endif


                /* read a2dp packet size */
                a2dp_packet_size = bulk_data_interface_read16bitWord_and_dont_mark_consumed(bulk_meta_data_ptr);

                /* check for EOF */
                if (a2dp_packet_size == EOF_MARKER)    {
                    /* we got an EOF, so end here */

                    buff_metadata_add_eof_callback(sinkBufMetaTag, a2dp_eof_callback,
                                            (unsigned)(uintptr_t)(&bulk_data_if->bulkifId));

                    #ifdef INSTALL_METADATA
                        /*
                         * this is the workaround for EOF not being handled correctly in
                         * RTP and SBC decoders yet. We directly delete the tag which triggers the
                         * callback immediately.
                         * ToDo: Fix this once B-252382 is fixed
                         */
                        buff_metadata_delete_tag(sinkBufMetaTag, TRUE);

                        /*
                         * a2dp_append_metadata_to_buf(a2dp->sink_buf, sinkBufMetaTag, 0, TRUE);
                         */
                    #endif

                    /* consume bulkif metadata */
                    bulk_data_interface_mark_consumed(bulk_meta_data_ptr, 2);
                    numRemainingA2dpPkts = 0;
                }
                else if ((a2dp_packet_size!=0) && (a2dp_packet_size<=available_bytes_in_sinkbuf))
                {
                    int copied = 0;


                    #ifndef UNIT_TEST_BUILD

                    copied = bulk_data_interface_copy_to_cbuf(bulk_data_ptr,
                             a2dp->sink_buf, a2dp_packet_size, BULKIF_CBUF_FORMAT_BIG_ENDIAN_16BIT);

                    #endif /* UNIT_TEST_BUILD */

                    /* sanitycheck if we were able to copy a full packet */
                    PL_ASSERT(a2dp_packet_size==copied);

                    #ifdef INSTALL_METADATA
                        a2dp_append_metadata_to_buf(a2dp->sink_buf, sinkBufMetaTag, a2dp_packet_size, FALSE);
                    #endif

                    /* consume bulkif metadata */
                    bulk_data_interface_mark_consumed(bulk_meta_data_ptr, 2);

                    numRemainingA2dpPkts--;
                    available_bytes_in_sinkbuf -= copied;
                }
                else
                {
                    /* *
                     * end here, we have created a tag earlier, but now we could not use it,
                     * so delete it here
                     * */
                    #ifdef INSTALL_METADATA
                        buff_metadata_delete_tag(sinkBufMetaTag,FALSE);
                    #endif

                    numRemainingA2dpPkts=0;
                }
            }
            while ((numRemainingA2dpPkts>0) && (available_bytes_in_sinkbuf<a2dp_packet_size));

            if (selfKick==TRUE)
            {
                /* Schedule a new event only if there isn't one pending */
                if (a2dp->self_kick_timer_id == TIMER_ID_INVALID)
                {
                    a2dp->self_kick_timer_id = timer_schedule_event_in(
                            A2DP_SELF_KICK_THRESHOLD,
                                    a2dp_self_kick,
                                     (void *)(ep));
                    A2DP_DBG_MSG1("a2dp Source: setting self kick timer (pending octets=%d)",
                            bulk_data_interface_get_fillLevelBytes(bulk_data_ptr));
                }
            }
            else
            {
                /* just reset our stall (watchdog) timer, since everything went ok */
                /* ToDo: Check if this is needed at all??? */
                timer_cancel_event(a2dp->stall_timer_id);
                a2dp->stall_timer_id = timer_schedule_bg_event_in(
                                                        a2dp_stall_threshold, a2dp_stall_handler,
                                                       (void*)ep);
                a2dp->stalled = FALSE;

                /**
                 * self_kick_timer_id could be TIMER_INVALID but that's
                 * taken care by timer_cancel_event.
                 */
                 timer_cancel_event(a2dp->self_kick_timer_id);
                 a2dp->self_kick_timer_id = TIMER_ID_INVALID;
            }
        }
        a2dp_source_kick_common(ep);
    }
    else
    {

#ifdef DEBUG_LOG_BUFFER
        if (!a2dpdbg) {
            a2dpdbg = (uint16 *)xzpmalloc(DBG_MAX_SIZE*sizeof(uint16));
        }
#endif
        /* if we have a an operator connected do further processing */
        if (ep->connected_to)
        {
#ifdef ADVANCED_DEBUG
            A2DP_DBG_MSG2("di_meta_prod=%d, bdi_meta_cons=%d",
                bulk_meta_data_ptr->producerOffset, bulk_meta_data_ptr->consumerOffset);
#endif

#if defined(A2DP_DEBUG_LOG)
            //metadata_tag *sinkBufMetaTag = NULL;
            unsigned available_bytes_in_sourcebuf = cbuffer_calc_amount_data_ex(a2dp->source_buf);
            if (BUFF_METADATA(a2dp->source_buf))
            {
                available_bytes_in_sourcebuf = buff_metadata_available_octets(a2dp->source_buf);
            }
#endif
            // local copy bulk data interface, bulk data interface producer offset is only written when the complete packet was written to memory
            sBdif_Info_t bulk_data_ptr_tmp;
            bulk_data_ptr_tmp.bufStartAddr = bulk_data_ptr->bufStartAddr;
            bulk_data_ptr_tmp.bufferSizeBytes = bulk_data_ptr->bufferSizeBytes;
            bulk_data_ptr_tmp.consumerOffset = bulk_data_ptr->consumerOffset;
            bulk_data_ptr_tmp.producerOffset = a2dp->producer_offset;


#ifdef DEBUG_AHB_BRIDGE
            uint16 tmp = bulk_meta_data_ptr->producerOffset;
            for (int i = 0; i < 1000; i++) {
                //bulk_meta_data_ptr->producerOffset += 1;//  = bulk_meta_data_ptr->producerOffset + 1;
                a2dpdbg[i] = bulk_meta_data_ptr->producerOffset;
            }
            // if (bulk_meta_data_ptr->producerOffset != tmp + 1000) {
            if (bulk_meta_data_ptr->producerOffset != tmp) {
                A2DP_DBG_ERRORMSG2("AHB error %d", bulk_meta_data_ptr->producerOffset, tmp);
            }
            bulk_meta_data_ptr->producerOffset = tmp;
#endif // DEBUG_AHB_BRIDGE

#ifdef ADVANCED_DEBUG
            /*debug*/
            static uint16 lastprodoffsetMeta=0;
            int diff = bulk_meta_data_ptr->producerOffset - lastprodoffsetMeta;
            if (diff < 0){
                diff += bulk_meta_data_ptr->bufferSizeBytes;
            }
            if (((diff) % 6) != 0) {
                A2DP_DBG_ERRORMSG1("Meta producer pointer not multiple of 6 %d", bulk_meta_data_ptr->producerOffset);
            }
            lastprodoffsetMeta = bulk_meta_data_ptr->producerOffset;


            /*debug*/
            static uint16 lastprodoffset = 0;
            diff = bulk_data_ptr->producerOffset - lastprodoffset;
            if (diff < 0){
                diff += bulk_data_ptr->bufferSizeBytes;
            }
            if (((diff) % 134) != 0) {
                A2DP_DBG_ERRORMSG1("producer pointer not multiple of 134 %d", bulk_data_ptr->producerOffset);
            }
            lastprodoffset = bulk_data_ptr->producerOffset;
#endif // ADVANCED_DEBUG



            /* the free data space depends on space in data buffer or space in metadata buffer
             * Since we cant distinguish between full or empty ptr offset we need to subtract one
             * additional byte in order not to get stalled */

            /* now check how much data space we have */
            uint32 free_bytes_bulk_data_interface = bulk_data_interface_get_numWriteableBytes(&bulk_data_ptr_tmp);

            /* now check how much metadata space we have */
            uint32 free_entries_in_bulk_meta_data_interface = bulk_data_interface_get_numWriteableBytes(bulk_meta_data_ptr);
            free_entries_in_bulk_meta_data_interface /= 6;

            if (free_bytes_bulk_data_interface > (free_entries_in_bulk_meta_data_interface * a2dp->mtu_size))
            {
                /* limit available bytes according to number of entries in metadata buffer */
                free_bytes_bulk_data_interface = free_entries_in_bulk_meta_data_interface * a2dp->mtu_size;
            }
#if defined(A2DP_DEBUG_LOG)
            A2DP_DBG_MSG3("A2DP Sink: Available bytes %d, free bytes in BDI %d, bytes written=%d", available_bytes_in_sourcebuf, free_bytes_bulk_data_interface, a2dp->bytes_written);
#endif
            /*
             * write data packets if available until mtu size is reached or until output buffer is full or until metadata buffer
             * is full
             */
            metadata_tag* tag = NULL;
#ifdef DEBUG_LOG_BUFFER
            int tagCnt = 0;
            a2dpdbg[dbgcnt + 1] = a2dp->mtu_size;
#endif // DEBUG_LOG_BUFFER

            do {
                tag = buff_metadata_peek(a2dp->source_buf);
                if (tag) {
#if defined(A2DP_DEBUG_LOG)
                    if (available_bytes_in_sourcebuf < tag->length) {
                        A2DP_DBG_MSG2("available_bytes_in_sourcebuf %d < tag->length %d", available_bytes_in_sourcebuf , tag->length);
                    }
#endif
                    /* sanity check that tag-length and packet_size stays constant */
                    if (tag->length!=a2dp->packet_size) {
                        A2DP_DBG_ERRORMSG2("A2DP Sink: tag->length= %d, a2dp->packet_size= %d", tag->length, a2dp->packet_size);
                    }
                    a2dp->packet_size = tag->length;
                    /* add header RTP header */
                    if (a2dp->bytes_written == 0) {
                        if (free_bytes_bulk_data_interface >= (sizeof(sRtpHeader_t)+1)) {

                            /* populate RTP header */
                            sRtpHeader_t *rtpHeader = (sRtpHeader_t*)& a2dp->rtp_header;
                            rtpHeader->header1.bits.version = 2;
                            unsigned padding = (a2dp->mtu_size - sizeof(sRtpHeader_t)-1) % a2dp->packet_size;
                            unsigned knPackets = (a2dp->mtu_size - sizeof(sRtpHeader_t)-1) / a2dp->packet_size;

                            if (padding == 0) {
                                rtpHeader->header1.bits.padding = 0;
                            }
                            else {
                                rtpHeader->header1.bits.padding = 1;
                            }

                            /* get number of samples from private meta data */
                            unsigned privateLength;
                            unsigned *privateData;
                            bool hasPrivateData=buff_metadata_find_private_data(tag, META_PRIV_KEY_USER_DATA, &privateLength, (void**) (&privateData));

                            /* rtpHeader->timeStamp=tag->timestamp;*/
                            if (hasPrivateData) {
                                rtpHeader->timeStamp += privateData[0];
                            }

                            a2dp->media_payload_header = knPackets;


                            bulk_data_interface_copy_from_lin_buf(&bulk_data_ptr_tmp, (uint8*)rtpHeader, sizeof(sRtpHeader_t)+1);
                            a2dp->bytes_written = a2dp->bytes_written + sizeof(sRtpHeader_t)+1;
                            free_bytes_bulk_data_interface -= (sizeof(sRtpHeader_t)+1);
                            rtpHeader->header1.bits.sequenceNumber += 1;
#ifdef ADVANCED_DEBUG
                            A2DP_DBG_MSG("A2DP Sink: Write RTP Header");
#endif /* ADVANCED_DEBUG */
                        }
                        else {
                            selfKick = TRUE;
                            break;
                        }
                    }



                    if (a2dp->bytes_written + a2dp->packet_size > a2dp->mtu_size) {         /* packet does not fit into mtu size */
                        break;
                    }
                    if (free_bytes_bulk_data_interface < a2dp->packet_size) {               /* not enough space in output buffer */
                        selfKick = TRUE;
                        break;
                    }

                    /* finally write packet */
                    A2DP_DBG_MSG1("A2DP Sink: Write Packet, size=%d", a2dp->packet_size);


                    /* for debug purpose (on Kalsim) print encoded cbuffer data
                     *  Don't use DBG_MSG because I want to print the plain data
                     */
#ifdef DEBUG_PRINT_ENCODED_FRAME
                    static int coffset = 0;
                    for (int i = 0; i < a2dp->packet_size*2; i++) {
                        if (i % 16 == 0) {
                            printf("\n");
                        }

                        printf("%2.2x ", *((uint8*)(((uint8*)a2dp->source_buf->base_addr) + coffset)));

                        coffset++;
                        if (coffset >= a2dp->source_buf->size) {
                            coffset = 0;
                        }
                    }
                    printf("\n");
#endif

                    if (a2dp->packet_size != 0) {
                        bulk_data_interface_copy_from_cbuf(&bulk_data_ptr_tmp, a2dp->source_buf, a2dp->packet_size, BULKIF_CBUF_FORMAT_BIG_ENDIAN_16BIT, a2dp->read_an_octet);
                        a2dp->read_an_octet = a2dp->read_an_octet ^ a2dp->packet_size;

                        a2dp->bytes_written += a2dp->packet_size;
                        free_bytes_bulk_data_interface -= a2dp->packet_size;
                    }
#ifdef DEBUG_LOG_BUFFER
                    a2dpdbg[dbgcnt + 2 + (tagCnt)] = tag->length;
                    a2dpdbg[dbgcnt + 2 + (tagCnt + 1)] = a2dp->bytes_written;
                    tagCnt+=2;
#endif /* DEBUG_LOG_BUFFER */

                    delete_consumed_metadata_tag(a2dp->source_buf, a2dp->packet_size);

                }
            } while (tag);

            /* check if we can finish this packet */
            if (a2dp->bytes_written + a2dp->packet_size >= a2dp->mtu_size){

                if (a2dp->bytes_written > a2dp->mtu_size) {
                    A2DP_DBG_ERRORMSG2("Packet Size too large (%d, %d)", a2dp->bytes_written, a2dp->mtu_size);
                }

                unsigned padding = a2dp->mtu_size - a2dp->bytes_written;

                if (free_bytes_bulk_data_interface < padding) {            /* not enough space in output buffer */
                    selfKick = TRUE;
                }
                else {
#ifdef ADVANCED_DEBUG
                    A2DP_DBG_MSG1("A2DP Sink: Write %d padding bytes", padding);
#endif /* ADVANCED_DEBUG */

                    if (padding>0){
#ifdef FILL_PADDING_BYTES
                        /* for debugging, fill the padding bytes with incremental number */
                        int i;
                        for (i = 1; i <= padding; i++){
                            bulk_data_interface_writeByte(&bulk_data_ptr_tmp, (uint8) i);
                        }
#else
                        /* increment write pointer by padding -1 (don't overwrite data and save the cycles, since this is ignored anyway) */
                        bulk_data_interface_mark_produced(&bulk_data_ptr_tmp, (uint16)(padding - 1));
                        /* write number of padding bytes into last byte*/
                        bulk_data_interface_writeByte(&bulk_data_ptr_tmp, (uint8)padding);
#endif
                    }

                    /* update real bulk date interface pointer*/
                    bulk_data_ptr->producerOffset = bulk_data_ptr_tmp.producerOffset;

                    /* write meta data (16bit mdu size)*/
                    bulk_data_interface_write16bitWord(bulk_meta_data_ptr, (uint16) a2dp->mtu_size);
                    /* write reserved bytes */
                    bulk_data_interface_write16bitWord(bulk_meta_data_ptr, (uint16)0x0000);
                    bulk_data_interface_write16bitWord(bulk_meta_data_ptr, (uint16)0x0000);

                    bulk_data_interface_peek16bitWord(bulk_meta_data_ptr);

                    /* send produced message to M0 */
                    bulk_data_interface_set_data_produced(bulk_data_if->bulkifId);

                    /* next call, start a new packet */
                    a2dp->bytes_written = 0;

#ifdef DEBUG_LOG_BUFFER

                    a2dpdbg[dbgcnt] = dbgcntAll;
                    a2dpdbg[dbgcnt+6] = bulk_data_ptr->producerOffset;
                    a2dpdbg[dbgcnt+7] = bulk_data_ptr->consumerOffset;
                    dbgcntAll++;
                    dbgcnt += 8;
                    if (dbgcnt >= DBG_MAX_SIZE-16) {
                        dbgcnt = 0;
                    }
                    for (int i = 0; i < 8; i++) {
                        a2dpdbg[dbgcnt + i] = 0x5555;
                    }
#endif /* DEBUG_LOG_BUFFER */
#ifdef ADVANCED_DEBUG
                    A2DP_DBG_MSG4("Packet written bdi_prod=%d, bdi_cons=%d, bdi_meta_prod=%d, bdi_meta_cons=%d",
                        bulk_data_ptr_tmp.producerOffset, bulk_data_ptr_tmp.consumerOffset,
                        bulk_meta_data_ptr->producerOffset, bulk_meta_data_ptr->consumerOffset);
#if defined(A2DP_DEBUG_LOG)
                    A2DP_DBG_MSG2("free_bytes_bulk_data_interface=%d, available_bytes_in_sourcebuf=%d",
                        free_bytes_bulk_data_interface, available_bytes_in_sourcebuf);
#endif
                    if ((bulk_meta_data_ptr->producerOffset & 1) == 1){
                        A2DP_DBG_ERRORMSG1("ODD OFFSET=%d", bulk_meta_data_ptr->producerOffset);
                    }
#endif /* ADVANCED_DEBUG */
                }
            }

            /* save producer offset for next call */
            a2dp->producer_offset=bulk_data_ptr_tmp.producerOffset;

            if (selfKick == TRUE)
            {
                /* Schedule a new event only if there isn't one pending */
                if (a2dp->self_kick_timer_id == TIMER_ID_INVALID)
                {
                    a2dp->self_kick_timer_id = timer_schedule_event_in(
                        A2DP_SELF_KICK_THRESHOLD,
                        a2dp_self_kick,
                        (void *)(ep));
                    A2DP_DBG_MSG1("a2dp Sink: setting self kick timer (pending octets=%d)",
                        bulk_data_interface_get_fillLevelBytes(bulk_data_ptr));
                }
            }
        }
        /* kick backwards to allow encoder to empty its buffer ... */
        ep->connected_to->functions->kick(ep->connected_to, STREAM_KICK_BACKWARDS);
    }
}



/**
 * \brief start the a2dp ep
 * \param ep pointer to the endpoint being started
 * \param ko pointer to the kick object that called start
 * \return TRUE if the call succeeded, FALSE otherwise
 */
static bool a2dp_start(ENDPOINT *ep, KICK_OBJECT *ko)
{
    patch_fn_shared(stream_a2dp);
    endpoint_a2dp_state *a2dp = &ep->state.a2dp;
    if (a2dp_start_common(ep))
    {
        /* Schedule the stall detection timer
         * Use casual events (through bg int)
         */
        a2dp->stall_timer_id = timer_schedule_bg_event_in(a2dp_stall_threshold, a2dp_stall_handler, (void*)ep);
        a2dp->self_kick_timer_id = TIMER_ID_INVALID;
        /* Get data moving so upstream doesn't give up */
        a2dp_kick(ep, STREAM_KICK_INTERNAL);
        a2dp->running = TRUE;
        ((sBdif_Interface_t*)a2dp->bulk_data_if)->callback_data = (void*)ko;
        ((sBdif_Interface_t*)a2dp->bulk_data_if)->handler = stream_a2dp_service_routine;
        /* The endpoint state must be configured before this call to start
         * consumer kicks in case the interrupt calls the kick handler before
         * this call is complete. */
        /* TODO_NAPIER: do we need to start the bulkif */
    }
    return TRUE;
}

/**
 * \brief stop the a2dp ep
 * \param ep pointer to the endpoint being stopped
 * \return TRUE if the call succeeded, FALSE otherwise
 */
static bool a2dp_stop(ENDPOINT *ep)
{
    patch_fn_shared(stream_a2dp);
    endpoint_a2dp_state *a2dp = &ep->state.a2dp;
    if (a2dp->running)
    {
        /* TODO_NAPIER: do we need to stop bulk_if? */

        /* The kick source is stopped before cancelling
         * the timer so we need not lock interrupts when the
         * timer is being cancelled. It is also assumed that
         * host command processing is run at lower priority
         * than a bus interrupt */
        timer_cancel_event(a2dp->stall_timer_id);
        timer_cancel_event(a2dp->self_kick_timer_id);
        a2dp->stall_timer_id = TIMER_ID_INVALID;
        a2dp->self_kick_timer_id = TIMER_ID_INVALID;
        a2dp->running = FALSE;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static void a2dp_stall_handler(void *ep)
{
    patch_fn_shared(stream_a2dp_kick);
    endpoint_a2dp_state *a2dp = &(((ENDPOINT *)ep)->state.a2dp);
    a2dp->stalled = TRUE;
}

/**
 * \brief callback executed when bulkd data if has been created.
 * \param sid internal endpoint id
 * \param status return from bulkif
 */
static void stream_a2dp_post_create_callback(unsigned sid, uint16 status)
{
    ENDPOINT *endpoint = stream_endpoint_from_extern_id(sid);
    bool status_bool = FALSE;
    if(status==BULKIF_RSP_SUCCESS){
        status_bool = TRUE;
    }
    stream_if_ep_creation_complete(endpoint, status_bool);
}

/**
 * \brief callback executed when bulk data if has been closed.
 * \param sid internal endpoint id
 * \param status return from bulkif
 */
static void stream_a2dp_post_close_callback(unsigned sid, uint16 status)
{
    ENDPOINT *endpoint = stream_endpoint_from_extern_id(sid);
    /* By the time we reach this function we should have stopped everything from
     * running, so all we need to do is tidy up the endpoint.  */
    if (SOURCE == endpoint->direction)
    {
        cbuffer_destroy(endpoint->state.a2dp.source_buf);
    }
    else
    {
        cbuffer_destroy(endpoint->state.a2dp.sink_buf);
    }

    stream_destroy_endpoint(endpoint);
    stream_if_ep_close_complete(endpoint, (bool) status);
}

/**
 * \brief Stream interrupt call back routine that gets called from the comms ISR
 *
 * \param kick_object kick object associated with the stream monitor
 *
 */
void stream_a2dp_service_routine(void* kick_object)
{
    KICK_OBJECT* ko = (KICK_OBJECT*)kick_object;
    PL_PRINT_P0(TR_STREAM, "stream_a2dp_routine\n");

    /* Need to check whether our kick object is NULL, because unfortunate
     * timing could see an interrupt being handled just after we disabled
     * the endpoint/connection. */
    if (ko != NULL)
    {
        /* We need to kick the kick object, not the endpoint, because it's
         * the kick object which knows who has responsibility for the chain.
         */
        kick_obj_kick(ko);
    }
}

bool a2dp_configure_napier(ENDPOINT *endpoint, unsigned key, uint32 value)
{
    endpoint_a2dp_state *a2dp = &(((ENDPOINT *)endpoint)->state.a2dp);

    switch (key)
    {
        case STREAM_CONFIG_KEY_STREAM_SINK_SHUNT_L2CAP_ATU:
            a2dp->mtu_size = (unsigned)value;
            return TRUE;
        default:
            return a2dp_configure(endpoint, key, value);
    }
}

bool a2dp_connect_napier(ENDPOINT *endpoint, tCbuffer *cbuffer_ptr, ENDPOINT *ep_to_kick, bool* start_on_connect)
{
    /* Derive the cap ID from the operator endpoint ID */
    endpoint_a2dp_state *a2dp = &endpoint->state.a2dp;
    a2dp->operator_cap_id = (uint16)opmgr_get_op_capid(endpoint->connected_to->id);

    A2DP_DBG_MSG1("a2dp Sink: connected to operator: 0x%x", a2dp->operator_cap_id);

    /*
    if (capid == CAP_ID_SBC_ENCODER) {

    }
    */

    return a2dp_connect(endpoint, cbuffer_ptr, ep_to_kick, start_on_connect);
}


/**
* Deletes the consumed metadata tag.
*
* \param o_buff Pointer to the octet buffer from which the tag will be consumed.
* \param octets_consumed Octets consumed from the buffer. Must be only one tag.
*/
void delete_consumed_metadata_tag(tCbuffer *cbuff, unsigned int  octets_consumed)
{

    unsigned int octets_b4idx, octets_afteridx;

    metadata_tag *tag = buff_metadata_remove(cbuff, octets_consumed, &octets_b4idx, &octets_afteridx);

    /* get_next_tag_size checks if the tag is at the beginning of the buffer. Therefore
    * octets_b4idx will be 0.*/
    if ((tag == NULL) ||
        (octets_b4idx != 0) ||
        (octets_afteridx != octets_consumed))
    {
        /* Metadata went out of sync. */
        A2DP_DBG_ERRORMSG("Meta Data out of Sync");

        /* do we want a panic here? System should recover from that ... */
        //panic(PANIC_AUDIO_DEBUG_ASSERT_FAILED);
    }
    buff_metadata_delete_tag(tag, TRUE);
}
