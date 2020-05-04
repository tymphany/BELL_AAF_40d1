/****************************************************************************
 * %%fullcopyright(2016)
****************************************************************************/
/**
* \defgroup anc_tuning
* \file  anc_tuning_defs.h
* \ingroup capabilities
*
* ANC_TUNING capability private header file. <br>
*
 */

#ifndef _ANC_TUNING_PRIVATE_H_
#define _ANC_TUNING_PRIVATE_H_
/******************************************************************************
Include Files
*/
#include "anc_tuning.h"
#include "op_msg_utilities.h"
#include "anc_tuning_gen_c.h"

/****************************************************************************
 Private Constant Definitions
 */

/** default block size for this operator's terminals */
#define ANC_TUNING_DEFAULT_BLOCK_SIZE                   1

/** total number of supported terminals for capability */
#define ANC_TUNING_MAX_SINKS          8
#define ANC_TUNING_MAX_SOURCES        4

#define ANC_TUNING_SINK_USB_LEFT      0 /*can be any other backend device. USB used in the ANC tuning graph*/
#define ANC_TUNING_SINK_USB_RIGHT     1 
#define ANC_TUNING_SINK_FBMON_LEFT    2 /*reserve slots for FBMON tap. Always connected.*/   
#define ANC_TUNING_SINK_FBMON_RIGHT   3    
#define ANC_TUNING_SINK_MIC1_LEFT     4 /* must be connected to internal ADC. Analog or digital */
#define ANC_TUNING_SINK_MIC1_RIGHT    5
#define ANC_TUNING_SINK_MIC2_LEFT     6      
#define ANC_TUNING_SINK_MIC2_RIGHT    7

#define ANC_TUNING_SOURCE_USB_LEFT    0 /* can be any other backend device. USB used in the tuning graph */
#define ANC_TUNING_SOURCE_USB_RIGHT   1
#define ANC_TUNING_SOURCE_DAC_LEFT    2 /* must be connected to internal DAC */
#define ANC_TUNING_SOURCE_DAC_RIGHT   3

#define USB_SINK_MASK       0x3
/* 2R | 2L | 1R | 1L | FR | FL | UR | UL */ 
#define ANC_SINK_MASK_1MIC  0x14   /* 0 0 0 1 0 1 0 0 */
#define ANC_SINK_MASK_2MIC  0x54   /* 0 1 0 1 0 1 0 0 */

/****************************************************************************
Public Type Declarations
*/

typedef struct anc_sink anc_sink_t;
struct anc_sink
{
    anc_sink_t *next;
    ENDPOINT   *ep_handle;
    tCbuffer   *buffer;
    unsigned   *buffer_start;
    unsigned   *read_ptr;
    unsigned   buffer_size;
};

typedef struct anc_source anc_source_t;
struct anc_source
{
    anc_source_t *next;
    ENDPOINT     *ep_handle;
    tCbuffer     *buffer;
    anc_sink_t   *sink;
    unsigned     sink_index;
    unsigned     peak;
};

/* 
    Must manualy verify this structure matches with instance parameters
    in ANC_TUNING_PARAMETERS. We take this shortcut because CPS can only
    generate one struct per XML.
*/

typedef struct _tag_ANC_INST_PARAMS
{
    unsigned OFFSET_ANC_USECASE;
    unsigned OFFSET_FF_A_MIC_SENSITIVITY;
    unsigned OFFSET_FF_B_MIC_SENSITIVITY;
    unsigned OFFSET_FF_A_FE_GAIN;
    unsigned OFFSET_FF_B_FE_GAIN;
    unsigned OFFSET_SPKR_RECEIVE_SENSITIVITY;
    unsigned OFFSET_SPKR_RECEIVER_IMPEDANCE;
    unsigned OFFSET_SPKR_RECEIVER_PA_GAIN;
    unsigned OFFSET_FF_A_ENABLE;
    unsigned OFFSET_FF_B_ENABLE;
    unsigned OFFSET_FB_ENABLE;
    unsigned OFFSET_FFA_IN_ENABLE;
    unsigned OFFSET_FFB_IN_ENABLE;
    unsigned OFFSET_FF_A_INPUT_DEVICE;
    unsigned OFFSET_FF_B_INPUT_DEVICE;
    unsigned OFFSET_FB_MON;
    unsigned OFFSET_FF_OUT_ENABLE;
    unsigned OFFSET_SMLPF_ENABLE;
    unsigned OFFSET_FF_FLEX_ENABLE;
    unsigned OFFSET_FF_A_GAIN_ENABLE;
    unsigned OFFSET_FF_B_GAIN_ENABLE;
    unsigned OFFSET_FB_GAIN_ENABLE;
    unsigned OFFSET_FF_A_DCFLT_ENABLE;
    unsigned OFFSET_FF_B_DCFLT_ENABLE;
    unsigned OFFSET_DMIC_X2_FF_A_ENABLE;
    unsigned OFFSET_DMIC_X2_FF_B_ENABLE;
    unsigned OFFSET_ANC_FF_A_SHIFT;
    unsigned OFFSET_ANC_FF_B_SHIFT;
    unsigned OFFSET_ANC_FB_SHIFT;
    unsigned OFFSET_ANC_FF_A_COEFF[15];
    unsigned OFFSET_ANC_FF_A_GAIN_SCALE;
    unsigned OFFSET_ANC_FF_A_GAIN_SCALE_DEFAULT;
    unsigned OFFSET_ANC_FF_A_GAIN;
    unsigned OFFSET_ANC_FF_B_COEFF[15];
    unsigned OFFSET_ANC_FF_B_GAIN_SCALE;
    unsigned OFFSET_ANC_FF_B_GAIN_SCALE_DEFAULT;
    unsigned OFFSET_ANC_FF_B_GAIN;
    unsigned OFFSET_ANC_FB_COEFF[15];
    unsigned OFFSET_ANC_FB_GAIN_SCALE;
    unsigned OFFSET_ANC_FB_GAIN_SCALE_DEFAULT;
    unsigned OFFSET_ANC_FB_GAIN;
    unsigned OFFSET_ANC_FF_A_LPF_SHIFT0;
    unsigned OFFSET_ANC_FF_A_LPF_SHIFT1;
    unsigned OFFSET_ANC_FF_B_LPF_SHIFT0;
    unsigned OFFSET_ANC_FF_B_LPF_SHIFT1;
    unsigned OFFSET_ANC_FB_LPF_SHIFT0;
    unsigned OFFSET_ANC_FB_LPF_SHIFT1;
    unsigned OFFSET_FF_A_DCFLT_SHIFT;
    unsigned OFFSET_FF_B_DCFLT_SHIFT;
    unsigned OFFSET_SM_LPF_SHIFT;
}ANC_INST_PARAMS;

/* capability-specific extra operator data */
typedef struct anc_tuning_exop
{
    /* Common Parameter System (CPS) data */
    CPS_PARAM_DEF params_def;                         /* CPS control block */
    ANC_TUNING_PARAMETERS anc_tuning_cap_params;      /* Current Parameters */
    unsigned ReInitFlag;
    anc_sink_t sinks[ANC_TUNING_MAX_SINKS];
    anc_source_t sources[ANC_TUNING_MAX_SOURCES];
    anc_sink_t *first_sink;
    anc_source_t *first_source;
    unsigned connected_sinks;
    unsigned connected_sources;
    bool is_stereo;   /* set by config message - before connect/start*/
    bool is_two_mic;  /* set by config message - before connect/start*/
    bool connect_change;
    uint16 coeffs[15];
    unsigned fb_mon[2];

} ANC_TUNING_OP_DATA;

/*****************************************************************************
Private Function Definitions
*/



#endif /* _ANC_TUNING_PRIVATE_H_ */

