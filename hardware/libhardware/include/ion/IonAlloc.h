#ifndef ANDROID_IONALLOC_INTERFACE_CPP_H
#define ANDROID_IONALLOC_INTERFACE_CPP_H
#include <utils/KeyedVector.h>
extern "C" {
    #include <ion/ionalloc.h>
}
namespace android {
    class IonAlloc {
     public:
        int alloc(unsigned long size, enum _ion_heap_type type, ion_buffer_t *data);
        int free(ion_buffer_t data);

        int share(ion_buffer_t data, int *share_fd);

        int map(int share_fd, ion_buffer_t *data);
        int unmap(ion_buffer_t data);

        int cache_op(ion_buffer_t data, enum cache_op_type type);
        int perform(int operation, ...);
        void set_id(enum ion_module_id id);

        IonAlloc() {ion_open(getpagesize(), ION_NUM_MODULES, &mIon); }
        IonAlloc(unsigned long align, enum ion_module_id id) { ion_open(align, id, &mIon); }
        ~IonAlloc() {ion_close(mIon);}
    private:
        ion_device_t *mIon;
        KeyedVector<void *, void *> mIonHandleMap;
    };
}
#endif /* ANDROID_IONALLOC_INTERFACE_CPP_H */
