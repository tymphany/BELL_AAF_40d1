############################################################################
# CONFIDENTIAL
# Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
#
############################################################################
#
# COMPONENT:    makefile
# MODULE:       base_multi_chan_op
# DESCRIPTION:  Library makefile.
#
############################################################################


#########################################################################
# Define root directory (relative so we can be installed anywhere)
#########################################################################

BUILD_ROOT = ../../build
include $(BUILD_ROOT)/roots.mkf


#########################################################################
# Enter source files and directories and header directories here.
#
# Makerules will add the standard interface paths
#########################################################################

C_SRC = base_multi_chan_op.c
S_SRC = base_multi_chan_asm.asm

# invoke the structure sharing utility
GEN_ASM_HDRS += base_multi_chan_op.h
GEN_ASM_DEFS += multi_channel_def
GEN_ASM_DEFS += multi_chan_channel_struc
GEN_ASM_DEFS += offload_multi_chan_channel_struc
GEN_ASM_DEFS += OPERATOR_DATA

#########################################################################
# Enter final target file here (only 1 target should be specified)
#########################################################################

TARGET_EXE_ROOT =
TARGET_LIB_ROOT = base_multi_chan_op

#########################################################################
# Include the standard definitions and rules
#########################################################################

include $(BUILD_ROOT)/makerules.mkf

