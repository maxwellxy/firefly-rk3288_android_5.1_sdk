// 从 sf 中的 MyTransform 简化修改得到. 

#ifndef ANDROID_MY_TRANSFORM_H
#define ANDROID_MY_TRANSFORM_H

#include <stdint.h>
#include <sys/types.h>

#include <ui/Point.h>
#include <ui/Rect.h>
#include <ui/vec2.h>
#include <ui/vec3.h>

#include <hardware/hardware.h>

namespace android {

// ---------------------------------------------------------------------------

class MyTransform
{
public:
                    MyTransform();

            typedef enum orientation_flags {
                ROT_0   = 0x00000000,
                FLIP_H  = HAL_TRANSFORM_FLIP_H,
                FLIP_V  = HAL_TRANSFORM_FLIP_V,
                ROT_90  = HAL_TRANSFORM_ROT_90,
                ROT_180 = FLIP_H|FLIP_V,
                ROT_270 = ROT_180|ROT_90,
                ROT_INVALID = 0x80
            }   orientation_flags_t;
           explicit MyTransform(uint32_t orientation);      // orientation_flags_t

                    ~MyTransform();

            enum type_mask {
                IDENTITY            = 0,
                TRANSLATE           = 0x1,
                ROTATE              = 0x2,
                SCALE               = 0x4,
                UNKNOWN             = 0x8
            };

            // modify the transform
            void        reset();

            status_t    set(uint32_t flags, float w, float h);  // .

            // transform data
            // assumes the last row is < 0 , 0 , 1 >
            vec2 transform(const vec2& v) const;

            MyTransform operator * (const MyTransform& rhs) const;

            // for debugging
            void dump(const char* name) const;

private:
    struct mat33 {
        vec3 v[3];
        inline const vec3& operator [] (int i) const { return v[i]; }
        inline vec3& operator [] (int i) { return v[i]; }
    };

    enum { UNKNOWN_TYPE = 0x80000000 };

    uint32_t type() const;
    static bool absIsOne(float f);
    static bool isZero(float f);

    mat33               mMatrix;
    mutable uint32_t    mType;
};

// ---------------------------------------------------------------------------
}; // namespace android

#endif /* ANDROID_MY_TRANSFORM_H */


