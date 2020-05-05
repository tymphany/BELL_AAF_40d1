/*!
\copyright  Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       touch.h
\brief      Header file for touch sensor support
*/

#ifndef __TYM_TOUCH_H__
#define __TYM_TOUCH_H__

#include <task_list.h>
#include "domain_message.h"

/*! Enumeration of messages the proximity sensor can send to its clients */
enum touch_messages
{
    /*! The sensor has detected an object tap1. */
    TOUCH_MESSAGE_TAPx1 = TOUCH_MESSAGE_BASE,
    /*! The sensor has detected an object tap2 */
    TOUCH_MESSAGE_TAPx2,
    /*! The sensor has detected an object tap3 */    
    TOUCH_MESSAGE_TAPx3,
    /*! The sensor has detected an object swipe left */
    TOUCH_MESSAGE_SWIPEL,
    /*! The sensor has detected an object swipe right */
    TOUCH_MESSAGE_SWIPER,
    /*! The sensor has detected an object hold 2s */
    TOUCH_MESSAGE_HOLD2S,
    /*! The sensor has detected an object hold 5s */    
    TOUCH_MESSAGE_HOLD5S,
    /*! The sensor has detected an object hold 10s */
    TOUCH_MESSAGE_HOLD10S,
    /*! The sensor has detected an object hold end for end voice trigger */     
    TOUCH_MESSAGE_HOLD2SEND,       
    /*! The sensor has detected an object hold end for end pairing trigger */     
    TOUCH_MESSAGE_HOLD5SEND,       
};

/*! Forward declaration of a config structure (type dependent) */
struct __touch_config;
/*! Proximity config incomplete type */
typedef struct __touch_config touchConfig;


/*! @brief Proximity module state. */
typedef struct
{
    /*! ready State module message task. */    
    TaskData task;
    /*! List of registered client tasks */
    task_list_t *clients;
    /*! The config */
    touchConfig *config;
    /*touch pad mode */
    uint8       touchPadMode;
} tymTouchTaskData;

/*!< Task information for proximity sensor */
extern tymTouchTaskData app_tymtouch;
/*! Get pointer to the proximity sensor data structure */
#define TymTouchGetTaskData()   (&app_tymtouch)

/*! \brief Register with proximity to receive notifications.
    \param task The task to register.
    \return TRUE if the client was successfully registered.
            FALSE if registration was unsuccessful or if the platform does not
            have a proximity sensor.
    The sensor will be enabled the first time a client registers.
*/
#if defined(INCLUDE_TOUCH)
extern bool appTouchClientRegister(Task task);
#else
#define appTouchClientRegister(task) FALSE
#endif

/*! \brief Unregister with proximity.
    \param task The task to unregister.
    The sensor will be disabled when the final client unregisters. */
#if defined(INCLUDE_TOUCH)
extern void appTouchClientUnregister(Task task);
#else
#define appTouchClientUnregister(task) ((void)task)
#endif

bool getFactoryTouchHoldLevel(void);
uint8 getFactoryTouchCh1threshold(void);
void updateTouchPadMode(void);
uint8 tymGetTouchPadMode(void);
void appTouchPowerOn(void);
void appTouchPowerOff(void);
#endif /* __TOUCH_H__ */
