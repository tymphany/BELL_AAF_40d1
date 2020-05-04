/****************************************************************************
 * Copyright (c) 2008 - 2017 Qualcomm Technologies International, Ltd.
 ****************************************************************************
 * \file sched_oxygen_for_dorm.h
 *
 * This header file is for use by dorm only.
 */

#ifndef SCHED_OXYGEN_FOR_DORM_H
#define SCHED_OXYGEN_FOR_DORM_H

/* Expose the TotalNumMessages global outside the scheduler to allow dorm to
 * interrogate it. */
extern volatile uint16f TotalNumMessages;

#endif /* SCHED_OXYGEN_FOR_DORM_H */
