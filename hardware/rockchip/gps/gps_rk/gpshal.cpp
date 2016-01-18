///////////////////////////////////////////////////////////////////////////////////
//
// Filename:     gpshal.cpp
// Author:       sjchen
// Copyright: 
// Date: 	2012/08/30
// Description:
//		GPS HAL layers
//
// Revision:
//		0.0.1
//
///////////////////////////////////////////////////////////////////////////////////
#include <hardware/hardware.h>
#include <hardware/gps.h>
#define LOG_TAG "HVGPSHAL"
#include <utils/Log.h>
#include <stdlib.h>
#include <errno.h>
#include "hvgps.h"

//below for 2.3 HAL device
struct gps_context_t 
{
    struct gps_device_t device;
    // our private state goes below here
};     
    
extern GpsInterface* gps_get_hardware_interface();  
static const GpsInterface *sGpsInterface = NULL;
static void gps_find_hardware(void)
{      
     sGpsInterface = gps_get_hardware_interface();
     if(!sGpsInterface)
        LOGD("No GPS hardware on this device");
}     

static const GpsInterface *gps_get_interface(struct gps_device_t* dev)          
{
      if(sGpsInterface == NULL)
         gps_find_hardware();
      return sGpsInterface;
}
    
         
static int gps_control_close(struct hw_device_t *dev)         
{
        struct gps_context_t *ctx = (struct gps_context_t *)dev;
        if(ctx){
             // free all resouces associated with this device here
             free(ctx);    
        }    
        return 0;
}
        
static int gps_device_open(const struct hw_module_t *module, const char *name, struct hw_device_t **device)  
{
        int status = -EINVAL;
        
        if (!strcmp(name, GPS_HARDWARE_MODULE_ID)) 
        {
            struct gps_context_t *dev;
            dev = (struct gps_context_t *)malloc(sizeof(*dev));
            if (dev == NULL) {
                return -ENOMEM;
            }             // initialize our state here
            
            memset(dev, 0, sizeof(*dev));             // initialize the procs
            dev->device.common.tag = HARDWARE_DEVICE_TAG;
            dev->device.common.version = 0;
            dev->device.common.module = (struct hw_module_t *) module;
            dev->device.common.close = gps_control_close;             
            dev->device.get_gps_interface = gps_get_interface;             
            
            *device = &dev->device.common;
            status = 0;
         }         
         return status;
}

struct gps_module_t 
{
    struct hw_module_t common;
};
      
static struct hw_module_methods_t gps_module_methods = 
{
         open: gps_device_open
}; 
    

struct gps_module_t HAL_MODULE_INFO_SYM = 
{
   common:{
            tag: HARDWARE_MODULE_TAG,
            version_major: 0,
            version_minor: 9,
            id: GPS_HARDWARE_MODULE_ID,
            name: "HighVision GPS Module",
            author: "sjchen",
            methods: &gps_module_methods  
   }  
};



 
