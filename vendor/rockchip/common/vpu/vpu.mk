LOCAL_PATH := $(call my-dir)

PRODUCT_PACKAGES += \
    libOMX_Core \
    libomxvpu_dec \
    libomxvpu_enc \
    librk_demux \
    librk_hevcdec \
    libRkOMX_Resourcemanager \
    librk_vpuapi

ifeq ($(filter sofia%, $(TARGET_BOARD_PLATFORM)), )
PRODUCT_PACKAGES += \
    libhevcdec \
    librkwmapro \
    librkswscale \
    libjesancache \
    libapedec\
    librk_audio \
    wfd
else
ifeq ($(strip $(USE_INTEL_MDP)), true)
PRODUCT_PACKAGES += \
    libmdp_omx_core \
    libstagefright_soft_aacdec_mdp \
    libstagefright_soft_aacenc_mdp \
    libstagefright_soft_amrdec_mdp \
    libstagefright_soft_amrenc_mdp \
    libstagefright_soft_mp3dec_mdp \
    libstagefright_soft_vorbisdec_mdp
endif
endif

ifeq ($(filter rk2928, $(TARGET_BOARD_PLATFORM)), )
PRODUCT_PACKAGES += \
    libjpeghwdec \
    libjpeghwenc
endif

ifneq ($(filter rk%, $(TARGET_BOARD_PLATFORM)), )
PRODUCT_PACKAGES += \
    librkffplayer \
    libffmpeg \
	iso
endif

PRODUCT_COPY_FILES += \
    vendor/rockchip/common/vpu/etc/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
    vendor/rockchip/common/vpu/etc/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml \
    vendor/rockchip/common/vpu/etc/media_codecs_rk_vpu.xml:system/etc/media_codecs_rk_vpu.xml
ifneq ($(filter rk312x rk3188, $(TARGET_BOARD_PLATFORM)), )
    PRODUCT_COPY_FILES += \
        vendor/rockchip/common/vpu/etc/media_codecs_rk312x.xml:system/etc/media_codecs.xml
else
    ifneq ($(filter rk%, $(TARGET_BOARD_PLATFORM)), )
        PRODUCT_COPY_FILES += \
            vendor/rockchip/common/vpu/etc/media_codecs.xml:system/etc/media_codecs.xml
    else
        PRODUCT_COPY_FILES += \
             vendor/rockchip/common/vpu/etc/media_codecs_sofia.xml:system/etc/media_codecs.xml
    endif
endif

