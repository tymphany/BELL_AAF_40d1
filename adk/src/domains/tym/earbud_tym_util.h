#ifndef __EARBUD_TYM_UTIL_H__
#define __EARBUD_TYM_UTIL_H__

/*! \brief Returns the PIOs bank number.
    \param pio The pio.
*/
#define PIO2BANK(pio) ((uint16)((pio) / 32))
/*! \brief Returns the PIO bit position mask within a bank.
    \param pio The pio.
*/
#define PIO2MASK(pio) (1UL << ((pio) % 32))

#define DEBUGBIT          (1 << 0)
#define PORTBIT           (1 << 1)
#define RESTOREBIT        (1 << 2)
#define PSDEBUG_MAX       3
#define ENABLE_UPDATE_AUDIO_PS_KEY  TRUE

typedef struct _tymDebugConfigData_s {
    unsigned config:3; // DEBUGBIT  (1 <<0), PORTBIT (1 << 1), RESTOREBIT (1<<2)
    unsigned reserve:13;//16
}tymDebugConfigData_s;

typedef struct _tymPresetEQ_s {
    unsigned eq:3;      // EQ preset 0~6
    unsigned reserve:13;
}tymPresetEQ_s;

bool checktimeout(uint32 oldtime,uint32 currtime,uint32 checktime);
void setupPIOInputAndInterrupt(uint8 pio);
void setupPIODisableInterrupt(uint8 pio);
void pioDriverPio(uint8 pin,bool enable);
void setPSDebugModeBit(uint8 bit,uint8 enable);
uint8 getPSDebugMode(void);

void setPSPresetEQ(void);
void getPSPresetEQ(void);
void UpdateAudioPSKey(void);
void configTYMRestoreDefault(void);
#endif


