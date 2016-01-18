
#include <math.h>

#include <cutils/compiler.h>
#include <utils/String8.h>
#include <ui/Region.h>

#include "clz.h"
#include "MyTransform.h"

// ---------------------------------------------------------------------------

namespace android {

// ---------------------------------------------------------------------------

MyTransform::MyTransform() {
    reset();
}

MyTransform::MyTransform(uint32_t orientation) {
    set(orientation, 0, 0);
}

MyTransform::~MyTransform() {
}

static const float EPSILON = 0.0f;

bool MyTransform::isZero(float f) {
    return fabs(f) <= EPSILON;
}

bool MyTransform::absIsOne(float f) {
    return isZero(fabs(f) - 1.0f);
}

MyTransform MyTransform::operator * (const MyTransform& rhs) const
{
    if (CC_LIKELY(mType == IDENTITY))
        return rhs;

    MyTransform r(*this);
    if (rhs.mType == IDENTITY)
        return r;

    // TODO: we could use mType to optimize the matrix multiply
    const mat33& A(mMatrix);
    const mat33& B(rhs.mMatrix);
          mat33& D(r.mMatrix);
    for (int i=0 ; i<3 ; i++) {
        const float v0 = A[0][i];
        const float v1 = A[1][i];
        const float v2 = A[2][i];
        D[0][i] = v0*B[0][0] + v1*B[0][1] + v2*B[0][2];
        D[1][i] = v0*B[1][0] + v1*B[1][1] + v2*B[1][2];
        D[2][i] = v0*B[2][0] + v1*B[2][1] + v2*B[2][2];
    }
    r.mType |= rhs.mType;

    // TODO: we could recompute this value from r and rhs
    r.mType &= 0xFF;
    r.mType |= UNKNOWN_TYPE;
    return r;
}

void MyTransform::reset() {
    mType = IDENTITY;
    for(int i=0 ; i<3 ; i++) {
        vec3& v(mMatrix[i]);
        for (int j=0 ; j<3 ; j++)
            v[j] = ((i==j) ? 1.0f : 0.0f);
    }
}

status_t MyTransform::set(uint32_t flags, float w, float h)
{
    if (flags & ROT_INVALID) {
        // that's not allowed!
        reset();
        return BAD_VALUE;
    }

    MyTransform H, V, R;
    if (flags & ROT_90) {
        // w & h are inverted when rotating by 90 degrees
        swap(w, h);
    }

    if (flags & FLIP_H) {
        H.mType = (FLIP_H << 8) | SCALE;
        H.mType |= isZero(w) ? IDENTITY : TRANSLATE;
        mat33& M(H.mMatrix);
        M[0][0] = -1;
        M[2][0] = w;
    }

    if (flags & FLIP_V) {
        V.mType = (FLIP_V << 8) | SCALE;
        V.mType |= isZero(h) ? IDENTITY : TRANSLATE;
        mat33& M(V.mMatrix);
        M[1][1] = -1;
        M[2][1] = h;
    }

    if (flags & ROT_90) {
        const float original_w = h;
        R.mType = (ROT_90 << 8) | ROTATE;
        R.mType |= isZero(original_w) ? IDENTITY : TRANSLATE;
        mat33& M(R.mMatrix);
        M[0][0] = 0;    M[1][0] =-1;    M[2][0] = original_w;
        M[0][1] = 1;    M[1][1] = 0;
    }

    *this = (R*(H*V));
    return NO_ERROR;
}

vec2 MyTransform::transform(const vec2& v) const {
    vec2 r;
    const mat33& M(mMatrix);
    r[0] = M[0][0]*v[0] + M[1][0]*v[1] + M[2][0];
    r[1] = M[0][1]*v[0] + M[1][1]*v[1] + M[2][1];
    return r;
}

uint32_t MyTransform::type() const
{
    if (mType & UNKNOWN_TYPE) {
        // recompute what this transform is

        const mat33& M(mMatrix);
        const float a = M[0][0];
        const float b = M[1][0];
        const float c = M[0][1];
        const float d = M[1][1];
        const float x = M[2][0];
        const float y = M[2][1];

        bool scale = false;
        uint32_t flags = ROT_0;
        if (isZero(b) && isZero(c)) {
            if (a<0)    flags |= FLIP_H;
            if (d<0)    flags |= FLIP_V;
            if (!absIsOne(a) || !absIsOne(d)) {
                scale = true;
            }
        } else if (isZero(a) && isZero(d)) {
            flags |= ROT_90;
            if (b>0)    flags |= FLIP_V;
            if (c<0)    flags |= FLIP_H;
            if (!absIsOne(b) || !absIsOne(c)) {
                scale = true;
            }
        } else {
            // there is a skew component and/or a non 90 degrees rotation
            flags = ROT_INVALID;
        }

        mType = flags << 8;
        if (flags & ROT_INVALID) {
            mType |= UNKNOWN;
        } else {
            if ((flags & ROT_90) || ((flags & ROT_180) == ROT_180))
                mType |= ROTATE;
            if (flags & FLIP_H)
                mType ^= SCALE;
            if (flags & FLIP_V)
                mType ^= SCALE;
            if (scale)
                mType |= SCALE;
        }

        if (!isZero(x) || !isZero(y))
            mType |= TRANSLATE;
    }
    return mType;
}

void MyTransform::dump(const char* name) const
{
    type(); // updates the type

    String8 flags, type;
    const mat33& m(mMatrix);
    uint32_t orient = mType >> 8;

    if (orient&ROT_INVALID) {
        flags.append("ROT_INVALID ");
    } else {
        if (orient&ROT_90) {
            flags.append("ROT_90 ");
        } else {
            flags.append("ROT_0 ");
        }
        if (orient&FLIP_V)
            flags.append("FLIP_V ");
        if (orient&FLIP_H)
            flags.append("FLIP_H ");
    }

    if (!(mType&(SCALE|ROTATE|TRANSLATE)))
        type.append("IDENTITY ");
    if (mType&SCALE)
        type.append("SCALE ");
    if (mType&ROTATE)
        type.append("ROTATE ");
    if (mType&TRANSLATE)
        type.append("TRANSLATE ");

    ALOGD("%s 0x%08x (%s, %s)", name, mType, flags.string(), type.string());
    ALOGD("%.4f  %.4f  %.4f", m[0][0], m[1][0], m[2][0]);
    ALOGD("%.4f  %.4f  %.4f", m[0][1], m[1][1], m[2][1]);
    ALOGD("%.4f  %.4f  %.4f", m[0][2], m[1][2], m[2][2]);
}

// ---------------------------------------------------------------------------

}; // namespace android
