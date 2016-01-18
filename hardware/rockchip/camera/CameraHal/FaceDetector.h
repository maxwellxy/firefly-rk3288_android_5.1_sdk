/*
 * Copyright 2011, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __FACE_DETECTOR_H_
#define __FACE_DETECTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

// Detector Type
enum {
    DETECTOR_OPENCV = 0,
    DETECTOR_OPENCL,
};

// Image Format
enum {
    IMAGE_RGBA8888 = 0,
    IMAGE_GRAYSCALE,
    IMAGE_YUV420SP,
    IMAGE_YUV420P,
};

// Image Orientation
enum {
    ROTATION_0 = 0,
    ROTATION_90,
    ROTATION_180,
    ROTATION_270,
};

struct RectFace {
    int x;
    int y;
    int width;
    int height;
};

#if 0
/*
  @type: Detector Type.
  @width: Initialize Image Width.
  @height: Initialize Image Height.
  @format: Image Format.
  @threshold: Detector Threshold, default = 10.0f.
*/
void FaceDetector_start(int type, int width, int height, int format, float threshold);

void FaceDetector_stop();

/*
  @src: Image Data.
  @orientation: Image Orientation.
  @faces: Output Find Face Rects.
  @num: Output Find Face Number.
*/
int FaceDetector_findFaces(void* src, int orientation, int isDrawRect, struct RectFace** faces, int* num);
#endif

#ifdef __cplusplus
}
#endif

#endif
