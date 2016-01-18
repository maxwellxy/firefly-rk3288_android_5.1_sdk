/*
 *
 * Copyright 2015 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file        gralloc_pirv_omx.cpp
 * @brief       get gpu private handle for omx use
 * @author      csy(csy@rock-chips.com)
 * @version     1.0.0
 * @history
 *   2015.1.16 : Create
 */

#include <cutils/log.h>
#include <hardware/gralloc.h>
#include "gralloc_priv_omx.h"
#ifdef GPU_G6110
#include <hardware/img_gralloc_public.h>
#define private_handle_t tagIMG_native_handle_t
#else
#include <gralloc_priv.h>
#endif

int32_t Rockchip_get_gralloc_private(uint32_t *handle,gralloc_private_handle_t *private_hnd){
    if(private_hnd == NULL){
        return -1;
    }
    private_handle_t *priv_hnd = (private_handle_t *)handle;
    private_hnd->format = priv_hnd->format;
#ifdef GPU_G6110
    private_hnd->share_fd = priv_hnd->fd[0];
#else
    private_hnd->share_fd = priv_hnd->share_fd;
#endif
    private_hnd->stride = priv_hnd->stride;
    private_hnd->type = priv_hnd->type;
    return 0;
}

