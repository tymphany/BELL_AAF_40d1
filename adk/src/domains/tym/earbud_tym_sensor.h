#ifndef __EARBUD_TYM_SENSOR_H__
#define __EARBUD_TYM_SENSOR_H__

typedef enum{
    notwear,
    wear,
}wearState_e;

typedef enum{
	noact,
    tap1,
    tap2,
    tap3,
    swipL,
    swipR,
    hold2s,
    hold5s,
    hold10s,
    slaveAct = 0x10,
    leftBud  = 0x20,
}actstatus_e;

typedef enum{
    ignorePad,
    normalPad,
    btPairingPad,
    restoreDefaultPad,
    sleepPad,
    standbyPad,
}touchpadmode_e;

#endif/*__EARBUD_TYM_SENSOR_H__*/
