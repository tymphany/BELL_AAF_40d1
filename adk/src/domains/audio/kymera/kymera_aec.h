/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Private header to connect/manage to AEC chain
*/

#ifndef KYMERA_AEC_H_
#define KYMERA_AEC_H_

#include <chain.h>
#include <operators.h>
//For Scrach Noise
#include "kymera_ucid.h"
//For Scrach Noise

#define AEC_REF_STF_GAIN_EXP_PARAM_INDEX 14
#define AEC_REF_STF_GAIN_MANTISSA_PARAM_INDEX 15

#define AEC_REF_CONFIG_PARAM_INDEX 0x0000
#define AEC_REF_CONFIG_PARAM_DEFAULT 0x2080
#define AEC_REF_CONFIG_PARAM_ENABLE_SIDETONE 0x2090

#define AEC_REF_SAME_INPUT_OUTPUT_CLK_SOURCE 0x0008

typedef struct
{
    uint16 id;
    uint16 value;
} aec_ref_set_same_in_out_clk_src_msg_t;

typedef struct
{
    Source input_1;
    Sink   speaker_output_1;
    Sink   speaker_output_2;
} aec_connect_audio_output_t;

typedef struct
{
    Sink   reference_output;
    Source mic_input_1;
    Source mic_input_2;
    Sink   mic_output_1;
    Sink   mic_output_2;
} aec_connect_audio_input_t;

typedef struct
{
    uint32 ttp_delay;
    uint32 ttp_gate_delay;
    uint32 spk_sample_rate;
    uint32 mic_sample_rate;
    uint32 buffer_size;
    bool is_source_clock_same;
}aec_audio_config_t;

/* descibes for what use-case AEC is being used */
typedef enum
{
    aec_usecase_default,
    aec_usecase_voice_call,
    aec_usecase_voice_assistant,
    aec_usecase_standalone_leakthrough,
    aec_usecase_a2dp_leakthrough,
    aec_usecase_sco_leakthrough,
    aec_usecase_va_leakthrough,
} aec_usecase_t ;

/*! \brief Connect audio output source to AEC.
    \param params All parameters required to configure/connect to the AEC chain.
    \note Handles the creation of the AEC chain.
*/
void Kymera_ConnectAudioOutputToAec(const aec_connect_audio_output_t *params, const aec_audio_config_t* config);

/*! \brief Disconnect audio output source from AEC.
    \note Handles the destruction of the AEC chain.
*/
void Kymera_DisconnectAudioOutputFromAec(void);

/*! \brief Connect audio input source to AEC.
    \param params All parameters required to configure/connect to the AEC chain.
    \note Handles the creation of the AEC chain.
*/
void Kymera_ConnectAudioInputToAec(const aec_connect_audio_input_t *params, const aec_audio_config_t* config);

/*! \brief Disconnect audio input source from AEC.
    \note Handles the destruction of the AEC chain.
*/
void Kymera_DisconnectAudioInputFromAec(void);

/*! \brief Get AEC Operator
*/
Operator Kymera_GetAecOperator(void);

/*! \brief Sets the AEC reference use-case
*/
void Kymera_SetAecUseCase(aec_usecase_t usecase);

/*! \brief Enable the sidetone path for AEC */
void Kymera_AecEnableSidetonePath(bool enable);

/*! \brief set Sidetone Gain for AEC */
void Kymera_AecSetSidetoneGain(uint32 exponent_value, uint32 mantissa_value);

/*! \brief get UCID for AEC_REF operator */
kymera_operator_ucid_t Kymera_GetAecUcid(void);

/*! \brief Facilitate transition to low power mode for AEC
*/
void Kymera_AecSleep(void);

/*! \brief Facilitate transition to exit low power mode for AEC
*/
void Kymera_AecWake(void);

#endif /* KYMERA_AEC_H_ */
