/*
 * Copyright (C) 2013 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
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

// #define ENABLE_DEBUG_LOG
#include <log/custom_log.h>

#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <sys/ioctl.h>

#include "alloc_device.h"
#include "gralloc_priv.h"
#include "gralloc_helper.h"
#include "framebuffer_device.h"

#include <linux/ion.h>
#include <ion/ion.h>
#include <linux/rockchip_ion.h>
extern int g_MMU_stat;
int alloc_backend_alloc(alloc_device_t* dev, size_t size, int usage, buffer_handle_t* pHandle)
{
	private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);
	ion_user_handle_t ion_hnd;
	unsigned char *cpu_ptr = NULL;
	int shared_fd;
	int ret;
	unsigned int heap_mask;
    int Ion_type;
    bool Ishwc = false;//
    int lock_state = 0;

	/*
	 * The following switch statement is intended to support the use of
	 * platform specific ION heaps using the gralloc private usage
	 * flags.
	 */
	switch((GRALLOC_USAGE_PRIVATE_2 | GRALLOC_USAGE_PRIVATE_3) & usage)
	{
	/* Example ion heap choice customization. */
	/*
	 *case GRALLOC_USAGE_PRIVATE_3:
	 *	heap_mask = ION_HEAP_TYPE_CARVEOUT;
	 *	break;
	 */
	default:
		//heap_mask = ION_HEAP_SYSTEM_MASK;
		//ALOGD("g_MMU_stat =%d",g_MMU_stat); 
		if(g_MMU_stat)
		{
            heap_mask = ION_HEAP(ION_VMALLOC_HEAP_ID);
            Ion_type = 1;
        }    
		else
		{
            heap_mask = ION_HEAP(ION_CMA_HEAP_ID);
            Ion_type = 0;
        }    
		break;
	}

	int ion_flags = 0;
    if(usage == (GRALLOC_USAGE_HW_COMPOSER|GRALLOC_USAGE_HW_RENDER|GRALLOC_USAGE_HW_VIDEO_ENCODER))
        Ishwc = true;

    #if 0
	if ( (usage & GRALLOC_USAGE_SW_READ_MASK) == GRALLOC_USAGE_SW_READ_OFTEN )
	{
		//ion_flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC; // Temporarily ignore,for ion dont supprot
	}

    if(usage == (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_SW_READ_OFTEN ))
    {
        heap_mask = ION_HEAP(ION_SYSTEM_HEAP_ID); // force Brower GraphicBufferAllocator to logics memery
    }
    #endif

    ALOGV("[%d,%d,%d],usage=%x",m->ion_client, size, ion_flags,usage);   
    ret = ion_alloc(m->ion_client, size, 0, heap_mask, ion_flags, &ion_hnd );
    if ( ret != 0 ) 
    {
        if( heap_mask == ION_HEAP(ION_CMA_HEAP_ID) && !Ishwc)
        {
            heap_mask = ION_HEAP(ION_VMALLOC_HEAP_ID);	 
            ret = ion_alloc(m->ion_client, size, 0, heap_mask, ion_flags, &ion_hnd );
            {
                if ( ret != 0) 
                {
                    AERR("Force to VMALLOC fail ion_client:%d", m->ion_client);
                    return -1;
                }
                else
                {
                    ALOGD("Force to VMALLOC sucess !");
                    Ion_type = 1;
                }        	            	    
            }
        }
        else
        {
            AERR("Failed to ion_alloc from ion_client:%d", m->ion_client);
            return -1;
        }	
    }

	ret = ion_share( m->ion_client, ion_hnd, &shared_fd );
	if ( ret != 0 )
	{
		AERR( "ion_share( %d ) failed", m->ion_client );
		if ( 0 != ion_free( m->ion_client, ion_hnd ) ) AERR( "ion_free( %d ) failed", m->ion_client );		
		return -1;
	}

	if (!(usage & GRALLOC_USAGE_PROTECTED))
	{
		cpu_ptr = (unsigned char*)mmap( NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0 );

		if ( MAP_FAILED == cpu_ptr )
		{
			AERR( "ion_map( %d ) failed", m->ion_client );
			if ( 0 != ion_free( m->ion_client, ion_hnd ) ) AERR( "ion_free( %d ) failed", m->ion_client );
			close( shared_fd );
			return -1;
		}
		lock_state = private_handle_t::LOCK_STATE_MAPPED;
	}

	private_handle_t *hnd = new private_handle_t( private_handle_t::PRIV_FLAGS_USES_ION, usage, size, cpu_ptr,
	                                              lock_state );

	if ( NULL != hnd )
	{
		hnd->share_fd = shared_fd;
		hnd->ion_hnd = ion_hnd;
		hnd->type = Ion_type;
		*pHandle = hnd;
		if(hnd->type== 1)
		{
		    ALOGW(" Debugmem The fd=%d, in vmalloc !!!! Ishwc=%d",hnd->share_fd,Ishwc);
		}
		return 0;
	}
	else
	{
		AERR( "Gralloc out of mem for ion_client:%d", m->ion_client );
	}

	close( shared_fd );

	if(!(usage & GRALLOC_USAGE_PROTECTED))
	{
		ret = munmap( cpu_ptr, size );
		if ( 0 != ret ) AERR( "munmap failed for base:%p size: %zd", cpu_ptr, size );
	}

	ret = ion_free( m->ion_client, ion_hnd );
	if ( 0 != ret ) AERR( "ion_free( %d ) failed", m->ion_client );
	return -1;
}

int alloc_backend_alloc_framebuffer(private_module_t* m, private_handle_t* hnd)
{
//	struct fb_dmabuf_export fb_dma_buf;
	int res;
	int share_fd = -1;
	res =  ioctl( m->framebuffer->fd, /*FBIOGET_DMABUF*/0x5003, &share_fd );//ioctl( m->framebuffer->fd, FBIOGET_DMABUF, &fb_dma_buf );
	if(res == 0)
	{
		hnd->share_fd = share_fd;//hnd->share_fd = fb_dma_buf.fd;
	}
	else
	{
		AINF("FBIOGET_DMABUF ioctl failed(%d). See gralloc_priv.h and the integration manual for vendor framebuffer integration", res);
	}

	return 0;
}

void alloc_backend_alloc_free(private_handle_t const* hnd, private_module_t* m)
{
	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
	{
		return;
	}
	else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP)
	{
		AERR( "Can't free ump memory for handle:%p. Not supported.", hnd );
	}
	else if ( hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION )
	{
		/* Buffer might be unregistered already so we need to assure we have a valid handle*/
		if ( 0 != hnd->base )
		{
			if ( 0 != munmap( (void*)hnd->base, hnd->size ) ) AERR( "Failed to munmap handle %p", hnd );
		}
		close( hnd->share_fd );
		if ( 0 != ion_free( m->ion_client, hnd->ion_hnd ) ) AERR( "Failed to ion_free( ion_client: %d ion_hnd: %p )", m->ion_client, hnd->ion_hnd );
		memset( (void*)hnd, 0, sizeof( *hnd ) );
	}
}

int alloc_backend_open(alloc_device_t *dev)
{
	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
	m->ion_client = ion_open();
	if ( m->ion_client < 0 )
	{
		AERR( "ion_open failed with %s", strerror(errno) );
		return -1;
	}

	return 0;
}

int alloc_backend_close(struct hw_device_t *device)
{
	alloc_device_t* dev = reinterpret_cast<alloc_device_t*>(device);
	if (dev)
	{
		private_module_t *m = reinterpret_cast<private_module_t*>(device);
		if ( 0 != ion_close(m->ion_client) ) AERR( "Failed to close ion_client: %d", m->ion_client );
		close(m->ion_client);
		delete dev;
	}
	return 0;
}
