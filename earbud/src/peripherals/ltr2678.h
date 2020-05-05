#ifdef INCLUDE_PROXIMITY
#ifdef HAVE_LTR2678


/*LTR2678 ps sensor register*/
#define LTR2678_PS_CTRL			0x81
#define LTR2678_PS_LED			0x82
#define LTR2678_PS_PULSES		0x83
#define LTR2678_PS_MEAS_RATE	0x84
#define LTR2678_PART_ID			0x86
#define LTR2678_MANUFAC_ID		0x87
#define LTR2678_PS_STATUS		0x91
#define LTR2678_PS_DATA_0		0x92
#define LTR2678_PS_DATA_1		0x93
#define LTR2678_INTERRUPT		0x98
#define LTR2678_INTERRUPT_PST	0x99
#define LTR2678_PS_THRES_UP_0	0x9A
#define LTR2678_PS_THRES_UP_1	0x9B
#define LTR2678_PS_THRES_LOW_0	0x9C
#define LTR2678_PS_THRES_LOW_1	0x9D
#define LTR2678_PS_CAN_0		0x9E
#define LTR2678_PS_CAN_1		0x9F
#define LTR2678_LED_DRIVE		0xA4
#define LTR2678_MAIN_CTRL		0xAD
#define LTR2678_IR_AMBIENT		0xB6
#define LTR2678_DSS_CTRL		0xB7
/* LTR-2678 Registers */

#define PS_INTERRUPT_MODE		1
#define PS_THRES_UP             1200//720//0x0400	// To be changed based on real test data
#define PS_THRES_LOW            1000//270//0x0200	// To be changed based on real test data

#define PS_USE_OFFSET			1

#define LTR2678_SUCCESS         0
#define LTR2678_ERROR           0xFF

#define PS_16BIT         		0 // 1: 16bit, 0:11bit

#define FTN_BIT                (1 << 5)
#define NTF_BIT                (1 << 4)

/*! The high level configuration for taking measurement */
struct __proximity_config
{
    /*! Measurements higher than this value will result in the sensor considering
        it is in-proximity of an object */
    uint16 threshold_high;
    /*! Measurements lower than this value will result in the sensor considering
        it is not in-proximity of an object */
    uint16 threshold_low;   
    /*! offset value */ 
    uint16 offset;  
    /*! Interrupt PIO driven by the sensor */
    uint8 interrupt;  
    bool  init;
};


/*! Internal representation of proximity state */
enum proximity_states
{
    proximity_state_unknown,
    proximity_state_in_proximity,
    proximity_state_not_in_proximity
};

/*! Trivial state for storing in-proximity state */
struct __proximity_state
{
    /*! The sensor proximity state */
    enum proximity_states proximity;
};


#endif/*HAVE_LTR2678*/
#endif/*INCLUDE_PROXIMITY*/
