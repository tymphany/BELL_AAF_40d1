// Copyright 2018 Google LLC.
#ifndef GSOUND_TYPES_H
#define GSOUND_TYPES_H

#include <stdbool.h>
#include <string.h>

//
// NOTE: If this header is included _before_ app/bluestack/types.h, it will
//       include stdint.h, which will cause conflicts.
//
//       If this header is included _after_ app/bluestack/types.h, it will
//       not include stdint.h, which will be okay.
//
//

// b/118391704
// Type conflicts in Stretto sample app between stdint.h and types.h:
// "C:\qtil\ADK_QCC512x_QCC302x_WIN_6.3.0.154\tools\kcc\bin\..\include\target32\target\stdint.h"
//      line 13: Error: illegal redeclaration of 'int8_t' (RedeclaredGlobal)
// "..\..\..\..\installed_libs\include\firmware_qcc512x_qcc302x\app\bluestack\types.h"
//      line 32: Error: previous declaration of 'int8_t' (RedeclaredGlobal)
#ifndef BLUESTACK__TYPES_H
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifdef __cplusplus
}
#endif

#endif  // GSOUND_TYPES_H
