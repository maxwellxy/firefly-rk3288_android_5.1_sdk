#ifdef INV_USE_LEGACY_NAMES
#ifndef INV_MLOS_LEGACY_H__
#define INV_MLOS_LEGACY_H__
#define MLOSMalloc inv_malloc
#define MLOSFree inv_free
#define MLOSCreateMutex inv_create_mutex
#define MLOSLockMutex inv_lock_mutex
#define MLOSUnlockMutex inv_unlock_mutex
#define MLOSFOpen inv_fopen
#define MLOSFClose inv_fclose
#define MLOSDestroyMutex inv_destroy_mutex
#define MLOSSleep inv_sleep
#define MLOSGetTickCount inv_get_tick_count
#endif
#endif
