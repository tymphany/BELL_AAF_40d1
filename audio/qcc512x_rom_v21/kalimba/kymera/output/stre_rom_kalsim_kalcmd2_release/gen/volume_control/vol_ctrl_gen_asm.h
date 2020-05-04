// -----------------------------------------------------------------------------
// Copyright (c) 2020                  Qualcomm Technologies International, Ltd.
//
#ifndef __VOL_CTRL_GEN_ASM_H__
#define __VOL_CTRL_GEN_ASM_H__

// CodeBase IDs
.CONST $M.VOL_CTRL_VOL_V2PLUS_CAP_ID       	0x0048;
.CONST $M.VOL_CTRL_VOL_V2PLUS_ALT_CAP_ID_0       	0x4054;
.CONST $M.VOL_CTRL_VOL_V2PLUS_SAMPLE_RATE       	0;
.CONST $M.VOL_CTRL_VOL_V2PLUS_VERSION_MAJOR       	2;

// Constant Values
.CONST $M.VOL_CTRL.CONSTANT.NUM_CHANNELS              		0x00000008;
// Constants for Auxiliary priority routing
.CONST $M.VOL_CTRL.CONSTANT.AUX_PRIORITY_NUM_BITS     		0x00000005;
.CONST $M.VOL_CTRL.CONSTANT.AUX_NUM_PRIORITIES        		0x00000004;
.CONST $M.VOL_CTRL.CONSTANT.AUX_PRIORITY_MASK         		0x0000001F;
.CONST $M.VOL_CTRL.CONSTANT.AUX_PRIORITY_CHANNEL_MASK 		0x00000007;
.CONST $M.VOL_CTRL.CONSTANT.AUX_PRIORITY1_MASK        		0x0000001F;
.CONST $M.VOL_CTRL.CONSTANT.AUX_PRIORITY2_MASK        		0x000003E0;
.CONST $M.VOL_CTRL.CONSTANT.AUX_PRIORITY3_MASK        		0x00007C00;
.CONST $M.VOL_CTRL.CONSTANT.AUX_PRIORITY4_MASK        		0x000F8000;
.CONST $M.VOL_CTRL.CONSTANT.AUX_PRIORITY_MUTE_BIT     		0x00000008;
.CONST $M.VOL_CTRL.CONSTANT.AUX_PRIORITY_VALID_BIT    		0x00000010;
.CONST $M.VOL_CTRL.CONSTANT.CHAN_NDVC_ENABLE_BIT      		0x00100000;
.CONST $M.VOL_CTRL.CONSTANT.CHAN_BOOST_CLIP_ENABLE_BIT		0x00200000;
// Constants for controls
.CONST $M.VOL_CTRL.CONSTANT.POST_GAIN_CTRL            		0x00000020;
.CONST $M.VOL_CTRL.CONSTANT.MASTER_GAIN_CTRL          		0x00000021;
.CONST $M.VOL_CTRL.CONSTANT.MUTE_PERIOD_CTRL          		0x00000022;
.CONST $M.VOL_CTRL.CONSTANT.AUX_GAIN_CTRL1            		0x00000030;
.CONST $M.VOL_CTRL.CONSTANT.AUX_GAIN_CTRL2            		0x00000031;
.CONST $M.VOL_CTRL.CONSTANT.AUX_GAIN_CTRL3            		0x00000032;
.CONST $M.VOL_CTRL.CONSTANT.AUX_GAIN_CTRL4            		0x00000033;
.CONST $M.VOL_CTRL.CONSTANT.AUX_GAIN_CTRL5            		0x00000034;
.CONST $M.VOL_CTRL.CONSTANT.AUX_GAIN_CTRL6            		0x00000035;
.CONST $M.VOL_CTRL.CONSTANT.AUX_GAIN_CTRL7            		0x00000036;
.CONST $M.VOL_CTRL.CONSTANT.AUX_GAIN_CTRL8            		0x00000037;
.CONST $M.VOL_CTRL.CONSTANT.TRIM1_GAIN_CTRL           		0x00000010;
.CONST $M.VOL_CTRL.CONSTANT.TRIM2_GAIN_CTRL           		0x00000011;
.CONST $M.VOL_CTRL.CONSTANT.TRIM3_GAIN_CTRL           		0x00000012;
.CONST $M.VOL_CTRL.CONSTANT.TRIM4_GAIN_CTRL           		0x00000013;
.CONST $M.VOL_CTRL.CONSTANT.TRIM5_GAIN_CTRL           		0x00000014;
.CONST $M.VOL_CTRL.CONSTANT.TRIM6_GAIN_CTRL           		0x00000015;
.CONST $M.VOL_CTRL.CONSTANT.TRIM7_GAIN_CTRL           		0x00000016;
.CONST $M.VOL_CTRL.CONSTANT.TRIM8_GAIN_CTRL           		0x00000017;


// Piecewise Disables
.CONST $M.VOL_CTRL.CONFIG.SATURATEENA    		0x00000002;
.CONST $M.VOL_CTRL.CONFIG.SATURATESYNCENA		0x00000004;


// Statistic Block
.CONST $M.VOL_CTRL.STATUS.OVERRIDE_CONTROL		0*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.POST_GAIN       		1*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.MASTER_GAIN     		2*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.AUXILIARY_GAIN1 		3*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.AUXILIARY_GAIN2 		4*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.AUXILIARY_GAIN3 		5*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.AUXILIARY_GAIN4 		6*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.AUXILIARY_GAIN5 		7*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.AUXILIARY_GAIN6 		8*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.AUXILIARY_GAIN7 		9*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.AUXILIARY_GAIN8 		10*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.TRIM_1          		11*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.TRIM_2          		12*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.TRIM_3          		13*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.TRIM_4          		14*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.TRIM_5          		15*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.TRIM_6          		16*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.TRIM_7          		17*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.TRIM_8          		18*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.AUX_STATE       		19*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.STATUS.BLOCK_SIZE           	20;

// Override
.CONST $M.VOL_CTRL.CONTROL.STATIC_OVERRIDE		0;
.CONST $M.VOL_CTRL.CONTROL.VOL_OVERRIDE   		1;

// Parameter Block
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CONFIG                    		0*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX1_SCALE                		1*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX1_ATK_TC               		2*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX1_DEC_TC               		3*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX2_SCALE                		4*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX2_ATK_TC               		5*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX2_DEC_TC               		6*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX3_SCALE                		7*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX3_ATK_TC               		8*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX3_DEC_TC               		9*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX4_SCALE                		10*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX4_ATK_TC               		11*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX4_DEC_TC               		12*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX5_SCALE                		13*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX5_ATK_TC               		14*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX5_DEC_TC               		15*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX6_SCALE                		16*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX6_ATK_TC               		17*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX6_DEC_TC               		18*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX7_SCALE                		19*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX7_ATK_TC               		20*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX7_DEC_TC               		21*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX8_SCALE                		22*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX8_ATK_TC               		23*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_AUX8_DEC_TC               		24*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN1_AUX_ROUTE           		25*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN1_PRIORITY1_PRIM_SCALE		26*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN1_PRIORITY2_PRIM_SCALE		27*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN1_PRIORITY3_PRIM_SCALE		28*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN1_PRIORITY4_PRIM_SCALE		29*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN2_AUX_ROUTE           		30*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN2_PRIORITY1_PRIM_SCALE		31*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN2_PRIORITY2_PRIM_SCALE		32*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN2_PRIORITY3_PRIM_SCALE		33*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN2_PRIORITY4_PRIM_SCALE		34*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN3_AUX_ROUTE           		35*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN3_PRIORITY1_PRIM_SCALE		36*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN3_PRIORITY2_PRIM_SCALE		37*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN3_PRIORITY3_PRIM_SCALE		38*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN3_PRIORITY4_PRIM_SCALE		39*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN4_AUX_ROUTE           		40*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN4_PRIORITY1_PRIM_SCALE		41*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN4_PRIORITY2_PRIM_SCALE		42*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN4_PRIORITY3_PRIM_SCALE		43*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN4_PRIORITY4_PRIM_SCALE		44*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN5_AUX_ROUTE           		45*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN5_PRIORITY1_PRIM_SCALE		46*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN5_PRIORITY2_PRIM_SCALE		47*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN5_PRIORITY3_PRIM_SCALE		48*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN5_PRIORITY4_PRIM_SCALE		49*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN6_AUX_ROUTE           		50*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN6_PRIORITY1_PRIM_SCALE		51*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN6_PRIORITY2_PRIM_SCALE		52*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN6_PRIORITY3_PRIM_SCALE		53*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN6_PRIORITY4_PRIM_SCALE		54*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN7_AUX_ROUTE           		55*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN7_PRIORITY1_PRIM_SCALE		56*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN7_PRIORITY2_PRIM_SCALE		57*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN7_PRIORITY3_PRIM_SCALE		58*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN7_PRIORITY4_PRIM_SCALE		59*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN8_AUX_ROUTE           		60*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN8_PRIORITY1_PRIM_SCALE		61*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN8_PRIORITY2_PRIM_SCALE		62*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN8_PRIORITY3_PRIM_SCALE		63*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CHAN8_PRIORITY4_PRIM_SCALE		64*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_CLIP_POINT                		65*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_BOOST_CLIP_POINT          		66*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_BOOST                     		67*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_VOLUME_TC                 		68*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_LIMIT_THRESHOLD_LINEAR    		69*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_LIMIT_ADAPTATION_RATIO    		70*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.OFFSET_LIMIT_THRESHOLD_LOG       		71*ADDR_PER_WORD;
.CONST $M.VOL_CTRL.PARAMETERS.STRUCT_SIZE                     		72;


#endif // __VOL_CTRL_GEN_ASM_H__
