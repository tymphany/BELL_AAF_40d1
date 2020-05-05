#ifndef __EARBUD_TYM_FACTORY__
#define __EARBUD_TYM_FACTORY__

/*! \brief factory status */
typedef enum
{
    /*! factory mode disable */
    factory_disable = 0,//00
    /*! factory mode enable */
    factory_enable = 1,//01
    /*! factory top board boot-up */
    factory_top = 2,//10
    /*! factory mode enable and top board boot-up */
    factory_enable_top = 3,//11
    /*! rule has been called, generated an action and completed */
} factory_status_t;

void tymReceivedDataFromHost(uint8 *recv_ptr);
bool getFactoryModeEnable(void);
bool getFactoryModeTopEnable(void);
void setFactoryModeStatus(uint8 mode);
void updateFactoryVthmVolt(uint16 vol);
void updateFactoryTouchEvent(uint8 iqs_events);
#endif

