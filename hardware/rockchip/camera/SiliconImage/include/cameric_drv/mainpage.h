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
 * @file    mainpage.h
 *
 *****************************************************************************/

/**
 *
 * @mainpage General Concept 
 *
 * The CamerIC driver package provides a general application programming
 * interface (API) for configuring the CamerIC ISP hardware. This driver 
 * package does not incorporate any "intelligence", like gain or integration
 * time controlling. It only provides an abstraction level for accessing 
 * the CamerIC hardware. 
 *
 * The following figure illustrates a typical software architecture using 
 * the CamerIC driver.
 *
 * @image html usecase.png "General usecase of the CamerIC driver" width=0.5\textwidth
 * @image latex usecase.png "General usecase of the CamerIC driver" width=0.5\textwidth
 *
 * The CamerIC driver package uses an underlying Hardware Abstraction 
 * Layer (HAL) to access CamerIC registers (HalReadReg / HalWriteReg) and
 * to register or release interrupt service routines (HalConnectIrq / HalDisconnectIrq).
 * This functionality aims to support different hardware platforms. Only
 * the HAL needs to be ported to a new hardware architecture. In case of a Linux
 * Kernel driver the HAL implements a wrapper functionality for register_irq and
 * free_irq.
 *
 * The Camera-Engine a typical middleware in a Camera-Software-Architecture works 
 * on top of the CamerIC driver. It initializes, configures and controls the 
 * underlying hardware by calling CamerIC driver API functions. In case of a 
 * Linux implementation an IOCTL layer is included between the Camera-Engine 
 * and the CamerIC driver software.
 *
 * @section cameric_drv_init_section Creating and Releasing a CamerIC driver instance
 *
 * The following figure shows the lifecycle-diagram of a CamerIC driver
 * instance. Generally an instance can be in on of the following two states.
 *
 * @arg INIT/STOP state: in this state all configuration parameter, like
 * resolution or measuring window, are changeable
 *
 * @arg RUNNING state: in this state it's only allowed to change the dynamic
 * configuration parameter, like measurement window or measurement mode.
 *
 * @image html state.png "States of a CamerIC driver instance" width=0.75\textwidth
 * @image latex state.png "States of a CamerIC driver instance" width=0.75\textwidth
 *
 * An instance is created by @ref CamerIcDriverInit and released by @ref
 * CamerIcDriverRelease. After a static configuration of the CamerIC driver
 * instance, which includes the configuration of the input acquisition (resolution,
 * bayerpattern, cropping window, ... ) it can be started by calling @ref CamerIcDriverStart. 
 * If a statically reconfiguring or a releasing the driver instance necessary 
 * the instance needs to be stopped with calling @ref CamerIcDriverStop.
 *
 * The following figure presents the initialization flow chart of the CamerIC Driver. 
 * First the CamEngine fills in the configuration structure @ref CamerIcDrvConfig_s.
 * This includes the base address of the CamerIC mapped register table and a HAL handle.
 * Than the CamEngine calls @ref CamerIcDriverInit, which creates and initializes the
 * driver context.
 *
 * @image html uml.png "Initialization and Releasing of the CamerIC driver" width=0.75\textwidth
 * @image latex uml.png "Initialization and Releasing of the CamerIC driver" width=0.75\textwidth
 *
 * On success the CamerIcDriverInit returns the handle of the new created CamerIc driver
 * instance. This handle is returned in the configuration structure (see
 * DrvHandle in @ref CamerIcDrvConfig_s).  
 *
 * @section cameric_drv_config_section Running a CamerIC driver instance
 *
 * As already said a driver instance can be started after a static configuration with @ref
 * CamerIcDriverStart. In the following flowchart an static configuration is exemplary
 * applied by configuring the ISP input acquisition (@ref
 * CamerIcSetAcqProperties and @  CamerIcSetAcqResoulution).
 *
 * @image html uml_config.png "Running a CamerIC driver instance" width=0.6\textwidth
 * @image latex uml_config.png "Running a CamerIC driver instance" width=0.6\textwidth
 *
 * A complete overview about all static and dynamic configuration parameter can be found in
 * @ref static_config_param_list.
 *
 * @section cameric_drv_streaming_ection Start/Stop streaming in a CamerIC driver instance
 *
 * @image html uml_streaming.png "Start/Stop Image capturing or video streaming" width=0.8\textwidth
 * @image latex uml_streaming.png "Start/Stop Image capturing or video streaming" width=0.8\textwidth
 *
 * @section cameric_drv_buffer_section Framehandling
 *
 * The CamerIC MI module supports storage of main picture YCbCr, JPEG and raw data (8 or 12 bit) as well
 * as self picture YCbCr and RGB data. Y, Cb and Cr data are stored in system memory in
 * separate buffers (planar), in two buffers (semi planar) or one buffer (interleaved).
 *
 * Depending on which paths are enabled and which output format is selected, the MI module
 * can simultaneously output image data to up to 6 independent buffers. Further, the MI module
 * supports updating the buffer configuration seamlessly between two
 * consecutive frames without dropping a frame by using the shadow register concept.
 *
 * Double buffering is implemented using the shadow register concept. Always two buffers need to be
 * allocated in system memory. While one buffer is filled, a second shadow buffer needs to be provided.
 *
 * @section cameric_drv_modularization_section Modularization of the CamerIC driver
 *
 * @if CAMERIC_LITE
 *
 * The following image shows all CamrIC-Lite driver top-level modules (red bordered).
 * *
 * @image html cameric_lite.png "Software toplevel modules of the CamerIC lite driver" width=\textwidth
 * @image latex cameric_lite.png "Software toplevel modules of the CamerIC lite driver" width=\textwidth
 *
 * @arg CamerIC CPROC (color processing, @ref cameric_cproc_drv_api)
 * @arg CamerIC IE (image effects, @ref cameric_ie_drv_api)
 * @arg CamerIC SI (super impose, @ref cameric_si_drv_api)
 * @arg CamerIC JPE (jpeg encoder, @ref cameric_jpe_drv_api)
 * @arg CamerIC MI (memory interface, @ref cameric_mi_drv_api)
 *
 * @endif
 *
 * @if CAMERIC_FULL
 *
 * The following image shows all CamrIC driver top-level modules (red bordered).
 *
 * @image html cameric20MP.png "Software toplevel modules of the CamerIC driver" width=\textwidth
 * @image latex cameric20MP.png "Software toplevel modules of the CamerIC driver" width=\textwidth
 *
 * @arg CamerIC MIPI (@ref cameric_mipi_drv_api)
 * @arg CamerIC ISP  (@ref cameric_isp_drv_api)
 * @arg CamerIC CPROC (color processing, @ref cameric_cproc_drv_api)
 * @arg CamerIC IE (image effects, @ref cameric_ie_drv_api)
 * @arg CamerIC SI (super impose, @ref cameric_simp_drv_api)
 * @arg CamerIC JPE (jpeg encoder, @ref cameric_jpe_drv_api)
 * @arg CamerIC MI (memory interface, @ref cameric_mi_drv_api)
 *
 * @endif
 *
 * @section cameric_isp_drv_modularization_section Modularization of the CamerIC ISP driver
 *
 * The following image shows all CamrIC ISP driver modules (red bordered).
 *
 * @image html cameric20MP_isp_modules.png "Software modules of the CamerIC ISP driver" width=\textwidth
 * @image latex cameric20MP_isp_modules.png "Software modules of the CamerIC ISP driver" width=\textwidth
 *
 * @arg CamerIC ISP BLS (@ref cameric_isp_bls_drv_api)
 * @arg CamerIC ISP DEGAMMA (@ref cameric_isp_degamma_drv_api)
 * @arg CamerIC ISP LSC (@ref cameric_isp_lsc_drv_api)
 * @arg CamerIC ISP AWB (@ref cameric_isp_awb_drv_api)
 * @arg CamerIC ISP AFM (@ref cameric_isp_afm_drv_api)
 * @arg CamerIC ISP HIST (@ref cameric_isp_hist_drv_api)
 * @arg CamerIC ISP AE (@ref cameric_isp_exp_drv_api)
 * @arg CamerIC ISP DPCC (@ref cameric_isp_dpcc_drv_api)
 * @arg CamerIC ISP DPF (@ref cameric_isp_dpf_drv_api)
 * @arg CamerIC ISP FLT (@ref cameric_isp_flt_drv_api)
 * @arg CamerIC ISP CAC (@ref cameric_isp_cac_drv_api)
 * @arg CamerIC ISP WDR (@ref cameric_isp_wdr_drv_api)
 * @arg CamerIC ISP VSM (@ref cameric_isp_vsm_drv_api)
 * @arg CamerIC ISP IS (@ref cameric_isp_is_drv_api)
 *
 * @section cameric_3d_section CamerIC 3D configuration
 *
 * The standard CamerIC IP may be accompanied by a CamerIC slave IP which processes
 * the data of the 2nd video sensor. The camerIC slave can be a full featured CamerIC
 * or an area optimized version of the standard camerIC.
 *
 * @image html cameric3D.png "Example CamerIC 3D configuration" width=\textwidth
 * @image latex cameric3D.png "Example CamerIC 3D configuration" width=\textwidth
 *
 *****************************************************************************/

