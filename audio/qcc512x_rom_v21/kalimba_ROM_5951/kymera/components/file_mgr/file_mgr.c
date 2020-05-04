/*****************************************************************************
*
* Copyright 2017 Qualcomm Technologies International, Ltd.
*
*****************************************************************************/
/**
 * \file file_mgr.c
 * \ingroup file_mgr
 */
#ifdef INSTALL_FILE_MGR
/****************************************************************************
Include Files
*/

#include "file_mgr_private.h"

#ifdef AUDIO_SECOND_CORE
#include "ipc/ipc_procid.h"
#include "adaptor/adaptor.h"
#include "hal_hwsemaphore.h"
#endif

/****************************************************************************
Private Constant and macros
*/
#define HWSEM_FILEMGR_RETRIES     16

/****************************************************************************
Private Variable Definitions
*/

/* hash table where array index is the file id */
static DM_P0_RW_ZI DATA_FILE *stored_files[FILE_MGR_MAX_NUMBER_OF_FILES];

/****************************************************************************
Private Function Definitions
*/

#ifdef AUDIO_SECOND_CORE
static DATA_FILE *hwsem_get_stored_file(uint16 idx)
{
    DATA_FILE *file = NULL;

    if (hal_hwsemaphore_get_with_retry(HWSEMIDX_IPC_FILEMGR, HWSEM_FILEMGR_RETRIES))
    {
        file = stored_files[idx];
        hal_hwsemaphore_rel(HWSEMIDX_IPC_FILEMGR);
    }
    else
    {
        L0_DBG_MSG3("File manager (get_stored_file): semaphore access failed, idx=%d (sem# %d, retries %d)", idx, HWSEMIDX_IPC_FILEMGR, HWSEM_FILEMGR_RETRIES);
    }
    return file;
}
#else
static inline DATA_FILE *hwsem_get_stored_file(uint16 idx)
{
    return stored_files[idx];
}
#endif

#ifdef AUDIO_SECOND_CORE
static bool hwsem_set_stored_file(uint16 idx, DATA_FILE *file)
{
    if (hal_hwsemaphore_get_with_retry(HWSEMIDX_IPC_FILEMGR, HWSEM_FILEMGR_RETRIES))
    {
        stored_files[idx] = file;
        hal_hwsemaphore_rel(HWSEMIDX_IPC_FILEMGR);
        return TRUE;
    }
    else
    {
        L0_DBG_MSG3("File manager (set_stored_file): semaphore access failed, idx=%d (sem# %d, retries %d)", idx, HWSEMIDX_IPC_FILEMGR, HWSEM_FILEMGR_RETRIES);
    }
    return FALSE;
}
#else
static inline bool hwsem_set_stored_file(uint16 idx, DATA_FILE *file)
{
    stored_files[idx] = file;
    return TRUE;
}
#endif

/**
 * \brief Checks if the provided file ID is an acceptable file ID.
 *
 * \param file_id       - a file ID
 *
 * \return TRUE if the provided id is valid
 */
static bool file_mgr_is_valid_id(uint16 file_id)
{
    patch_fn_shared(file_mgr);

    /* Check if the id is in acceptable range and of the expected
     * type. The FILE_MGR_BASE_MASK (0xF000) is expected to be set,
     * as created by 'file_mgr_alloc_file'. Optionally, in addition
     * the FILE_MGR_AUTO_MASK bits may be set (0x0F00). 
     */
    if ((FILE_MGR_IS_NONAUTO(file_id) || FILE_MGR_IS_AUTO(file_id)) &&
        (FILE_ID_TO_INDEX(file_id) < FILE_MGR_MAX_NUMBER_OF_FILES )    )
    {
        /* Check if the file has been allocated */
        if (hwsem_get_stored_file(FILE_ID_TO_INDEX(file_id)) != NULL)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * \brief Frees allocated file memory.
 *
 * \param file_idx      - file index
 */
static void file_mgr_clean_up(uint16 file_idx)
{
    DATA_FILE *file;

    patch_fn_shared(file_mgr);

    file = hwsem_get_stored_file(file_idx);
    /* Pfree checks for null pointers so we do not have to worry */
    if (NULL != file)
    {
#ifdef INSTALL_EXTERNAL_MEM
        if (ACCMD_TYPE_OF_FILE_EDF == file->type)
        {
            ext_buffer_destroy(file->u.ext_file_data);
        }
        else
#endif /* INSTALL_EXTERNAL_MEM */
        {
            cbuffer_destroy(file->u.file_data);
        }
        pdelete(file);
    }

    /* Reset the pointers to NULL */
    hwsem_set_stored_file(file_idx, NULL);
}

/**
 * \brief Allocates a buffer of a requested size for a data file
 *
 * \param file_size     - Size of buffer to be reserved for the file (in octets)
 *
 * \return Audio Status code
 */
static uint16 file_mgr_alloc_file(uint32 file_size,
                                  ACCMD_TYPE_OF_FILE type,
                                  bool auto_free )
{
    uint16 file_idx;
    unsigned file_size_in_words;
    patch_fn_shared(file_mgr);

    for (file_idx = 0; file_idx < FILE_MGR_MAX_NUMBER_OF_FILES; file_idx++)
    {
        if (NULL == hwsem_get_stored_file(file_idx))
        {
            DATA_FILE *file = NULL;

            /* Convert size to words needed by the cbuffer */
            file_size_in_words = OCTETS_TO_CBUFFER_SIZE(file_size);

            /* allocate the file struct */
            file = xpnew(DATA_FILE);
            if (NULL == file)
            {
                /* Fail */
                fault_diatribe(FAULT_AUDIO_INSUFFICIENT_MEMORY,
                               sizeof(DATA_FILE));
                break;
            }

#ifdef INSTALL_EXTERNAL_MEM
            /* STORE IN SRAM */
            if (ACCMD_TYPE_OF_FILE_EDF == type)
            {
                /* get the ext_buffer */
                file->u.ext_file_data = ext_buffer_create(file_size_in_words,
                                                          EXT_BUFFER_CIRC_ACCESS_FLAG_MASK);
            }
            else
#endif /* INSTALL_EXTERNAL_MEM */
            {
                /* Get the cbuffer */
                file->u.file_data = cbuffer_create_with_malloc(file_size_in_words,
                                                               BUF_DESC_SW_BUFFER);
            }

            if (NULL == file->u.file_data)
            {
                fault_diatribe(FAULT_AUDIO_INSUFFICIENT_MEMORY, file_size);
                /* Clean up and fail */
                pdelete(file);
                break;
            }

            /* Set the file type */
            file->type = type;
            file->valid = FALSE;

            if (!hwsem_set_stored_file(file_idx, file))
            {
#ifdef INSTALL_EXTERNAL_MEM
                if (ACCMD_TYPE_OF_FILE_EDF == type)
                {
                    ext_buffer_destroy(file->u.ext_file_data);
                }
                else
#endif /* INSTALL_EXTERNAL_MEM */
                {
                    cbuffer_destroy(file->u.file_data);
                }
                pdelete(file);
            }
            else
            {
                /* Success return file ID */
                return (auto_free) ? INDEX_TO_AUTO_FILE_ID(file_idx):
                                     INDEX_TO_NON_AUTO_FILE_ID(file_idx);
            }
        }
    }
    /* File allocation failed */
    return FILE_MGR_INVALID_FILE_ID;
}

/**
 * \brief Deallocates a buffer of a requested size for a data file
 *
 * \param file_id       - ID of the file to be removed
 *
 * \return Audio Status code
 */
static bool file_mgr_dealloc_file(uint16 file_id)
{
    /* Convert file id to file index */
    uint16 file_idx;
    patch_fn_shared(file_mgr);

    if (TRUE == file_mgr_is_valid_id(file_id))
    {
        /* Get file index */
        file_idx = FILE_ID_TO_INDEX(file_id);

        /* Free allocated buffers */
        file_mgr_clean_up(file_idx);
        return TRUE;
    }
    return FALSE;
}

#ifdef AUDIO_SECOND_CORE
/**
 * \brief  Helper function for dispatching file manager release file
 *         requests to P0.
 *
 * \param  file_id    File ID (as created by 'file_mgr_alloc_file')
 *
 * \return TRUE if message sent successfully (P1->P0), FALSE if not
 */
static bool file_mgr_send_msg_file_mgr_release_file_query(uint16 file_id)
{
    uint16 kip_con_id;
    ADAPTOR_MSGID msg_id;
    KIP_MSG_FILE_MGR_RELEASE_FILE_REQ* req_msg;

    req_msg = xpnew(KIP_MSG_FILE_MGR_RELEASE_FILE_REQ);
    if (req_msg == NULL)
    {
        return FALSE;
    }

    kip_con_id = PACK_CON_ID(PACK_SEND_RECV_ID(IPC_PROCESSOR_0,0),
                             PACK_SEND_RECV_ID(IPC_PROCESSOR_1,0));
    msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_FILE_MGR_RELEASE_FILE_REQ);

    KIP_MSG_FILE_MGR_RELEASE_FILE_REQ_CON_ID_SET(req_msg, kip_con_id);
    KIP_MSG_FILE_MGR_RELEASE_FILE_REQ_FILE_ID_SET(req_msg, file_id);

    adaptor_send_message(kip_con_id,
                         msg_id,
                         KIP_MSG_FILE_MGR_RELEASE_FILE_REQ_WORD_SIZE,
                         (ADAPTOR_DATA) req_msg);
    pfree(req_msg);

    return TRUE;
}
#endif /* AUDIO_SECOND_CORE */

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Allocates a buffer of a requested size for a data file and calls
 *        accmd response callback
 *
 * \param con_id        - Connection id
 * \param type          - Type of file
 * \param auto_free     - Free the buffer internally after first use.
 * \param file_size     - Size of buffer to be reserved for the file (in octets)
 * \param cback         - Callback
 */
void file_mgr_accmd_alloc_file(unsigned con_id, 
                               ACCMD_TYPE_OF_FILE type,
                               bool auto_free,
                               uint32 file_size,
                               FILE_MGR_ALLOC_CBACK cback)
{
    uint16 file_id;
    unsigned status = STATUS_OK;
    patch_fn_shared(file_mgr);

    /* ACCMD style callback. It is used to return status and file_id */
    PL_ASSERT(cback != NULL);

    /* Allocate space for file */
    file_id = file_mgr_alloc_file(file_size, type, auto_free);
    if (FILE_MGR_INVALID_FILE_ID == file_id)
    {
        status = STATUS_CMD_FAILED;
    }

    cback(con_id, status, file_id);
}

/**
 * \brief Deallocates a buffer of a requested size for a data file
 *        and calls accmd response callback
 *
 * \param con_id        - Connection id
 * \param file_id       - ID of the file to be removed)
 * \param cback         - Callback
 */
void file_mgr_accmd_dealloc_file(unsigned con_id, uint16 file_id,
                                 FILE_MGR_DEALLOC_CBACK cback)
{
    unsigned status = STATUS_OK;
    patch_fn_shared(file_mgr);

    /* ACCMD style callback. It is used to return status */
    PL_ASSERT(cback != NULL);

    /* Allocate space for file */
    if (file_mgr_dealloc_file(file_id) == FALSE)
    {
        /* There is no file with the requested ID */
        status = STATUS_INVALID_CMD_PARAMS;
    }

    cback(con_id, status);
}

/**
 * \brief Finds and returns a file of a given ID
 *
 * \param file_id       - ID of the file
 *
 * \return pointer to a file
 */
DATA_FILE * file_mgr_get_file(uint16 file_id)
{
    patch_fn_shared(file_mgr);

    if (file_mgr_is_valid_id(file_id))
    {
        /* If the id has not been allocated this will be NULL */
        return hwsem_get_stored_file(FILE_ID_TO_INDEX(file_id));
    }
    return NULL;
}

/**
 * \brief Release the file back for auto free files 
 *
 * \param file_id       - ID of the file
 *
 * \return TRUE if successfully released file from
 *         'stored_files', FALSE if not (file_id is
 *         not a valid entry in 'stored_files').
 */
bool file_mgr_release_file(uint16 file_id)
{
    patch_fn_shared(file_mgr);

#ifdef AUDIO_SECOND_CORE
    if (IPC_SECONDARY_CONTEXT())
    {
        return file_mgr_send_msg_file_mgr_release_file_query(file_id);
    }
#endif

    /* Release the file only if file_id is marked auto free */
    if (FILE_MGR_IS_AUTO_FREE(file_id))
    {
        if (file_mgr_dealloc_file(file_id) == FALSE)
        {
            /* There is no file with the requested ID */
            L0_DBG_MSG1("File manager (release_file): %d is not a valid file_id, unable to deallocate file", file_id);
            return FALSE;
        }
    }
    return TRUE;
}

/**     
 * \brief The audio data service calls this function
 *        once transfer is done
 *  
 * \param file          - The file transfered
 * \param file_id       - ID of the file to be removed)
 * \param cback         - Callback
 */
void file_mgr_transfer_done(DATA_FILE *file, bool status, 
                            FILE_MGR_EOF_CBACK cback)
{
    uint16 msg_id;
    patch_fn_shared(file_mgr);

    PL_ASSERT(file != NULL);

    msg_id = (status) ? FILE_MGR_EOF : FILE_MGR_ERROR;
    /* Update the file status here */
    file->valid = (status) ? TRUE : FALSE;

    put_message(FILE_MGR_TASK_QUEUE_ID, msg_id, (void *) cback);
}
                           
/**     
 * \brief Init function. 
 *        Doing nothing now. keeping it only for patching purpose.
 *        Called in 'boot.c'.
 */
void file_mgr_init(void)
{
    patch_fn_shared(file_mgr);
}

/**
 * \brief (File manager task's) init function.
 *        Used in file_mgr_sched.h for file manager static task.
 */
void file_mgr_task_init(void **data)
{
    patch_fn_shared(file_mgr);
}

/**
 * \brief (File manager task's) handler for the messages in the task queue.
 *        Used in file_mgr_sched.h for file manager static task
 *
 * \param  data         - Pointer to the pointer to task data
 */
void file_mgr_task_handler(void **data)
{

    file_mgr_message_id msg_id;
    void *msg_data;
    patch_fn_shared(file_mgr);

    while (get_message( FILE_MGR_TASK_QUEUE_ID, (uint16*)&msg_id, &msg_data))
    {
        switch (msg_id)
        {
            /* A file has been downloaded successfully */
             case FILE_MGR_EOF:
                {
                    FILE_MGR_EOF_CBACK callback = (FILE_MGR_EOF_CBACK) msg_data;
                    callback(TRUE);
                }
                break;

             case FILE_MGR_ERROR:
                {
                    FILE_MGR_EOF_CBACK callback = (FILE_MGR_EOF_CBACK) msg_data;
                    callback(FALSE);
                }
                break;
             default:
                break;
        }
    }
}

#endif /* INSTALL_FILE_MGR */
