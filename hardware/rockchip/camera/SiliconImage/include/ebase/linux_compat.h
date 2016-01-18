/******************************************************************************
 *
 * Copyright 2010, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @file linux_compat.h
 *
 * @brief
 *   Linux compatibility layer.
 *
 *****************************************************************************/
#ifndef LINUX_COMPAT_H_
#define LINUX_COMPAT_H_

#ifdef __cplusplus
	extern "C"
    {
#endif

/***** macro definitions *****************************************************/

#   ifdef __cplusplus
#       include <cstdio>
//#       include <cstdlib>  //conflict with stlport,zyc
#   else
#       include <stdio.h>
#       include <stdlib.h>
#   endif

/***** public type definitions ***********************************************/

/***** public function prototypes ********************************************/

#ifdef __cplusplus
	}
#endif
#endif /* LINUX_COMPAT_H_ */

