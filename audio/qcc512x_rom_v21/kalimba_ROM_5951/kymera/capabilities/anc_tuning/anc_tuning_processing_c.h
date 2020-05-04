/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  anc_tuning_processing_c.h
 * \ingroup  capabilities
 *
 *  Declaration of assembly functions used by the ANC tuning capability.
 */
 
#ifndef ANC_TUNING_PROCESSING_C_H
#define ANC_TUNING_PROCESSING_C_H

#include "anc_tuning_defs.h"

extern void anc_tuning_processing(ANC_TUNING_OP_DATA *p_ext_data, unsigned num_samples);

#endif /* ANC_TUNING_PROCESSING_C_H */