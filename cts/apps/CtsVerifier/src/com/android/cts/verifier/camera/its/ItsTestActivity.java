/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.cts.verifier.camera.its;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;
import java.util.HashSet;
import java.util.Arrays;

import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;


/**
 * Test for Camera features that require that the camera be aimed at a specific test scene.
 * This test activity requires a USB connection to a computer, and a corresponding host-side run of
 * the python scripts found in the CameraITS directory.
 */
public class ItsTestActivity extends PassFailButtons.Activity {
    private static final String TAG = "ItsTestActivity";
    private static final String EXTRA_SUCCESS = "camera.its.extra.SUCCESS";
    private static final String ACTION_ITS_RESULT =
            "com.android.cts.verifier.camera.its.ACTION_ITS_RESULT";

    class SuccessReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.i(TAG, "Received result for Camera ITS tests");
            if (ACTION_ITS_RESULT.equals(intent.getAction())) {
                String result = intent.getStringExtra(EXTRA_SUCCESS);
                String[] parts = result.split("=");
                if (parts.length != 2) {
                    Toast.makeText(ItsTestActivity.this,
                            "Received unknown ITS result string: " + result,
                            Toast.LENGTH_SHORT).show();
                }
                String cameraId = parts[0];
                boolean pass = parts[1].equals("True");
                if(pass) {
                    Log.i(TAG, "Received Camera " + cameraId + " ITS SUCCESS from host.");
                    mITSPassedCameraIds.add(cameraId);
                    if (mCameraIds != null &&
                            mITSPassedCameraIds.containsAll(Arrays.asList(mCameraIds))) {
                        ItsTestActivity.this.showToast(R.string.its_test_passed);
                        ItsTestActivity.this.getPassButton().setEnabled(true);
                    }
                } else {
                    Log.i(TAG, "Received Camera " + cameraId + " ITS FAILURE from host.");
                    ItsTestActivity.this.showToast(R.string.its_test_failed);
                }
            }
        }
    }

    private final SuccessReceiver mSuccessReceiver = new SuccessReceiver();
    private final HashSet<String> mITSPassedCameraIds = new HashSet<>();
    private String[] mCameraIds = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.its_main);
        setInfoResources(R.string.camera_its_test, R.string.camera_its_test_info, -1);
        setPassFailButtonClickListeners();
        getPassButton().setEnabled(false);
    }

    @Override
    protected void onResume() {
        super.onResume();
        CameraManager manager = (CameraManager) this.getSystemService(Context.CAMERA_SERVICE);
        if (manager == null) {
            showToast(R.string.no_camera_manager);
        } else {
            try {
                mCameraIds = manager.getCameraIdList();
                boolean allCamerasAreLegacy = true;
                for (String id : mCameraIds) {
                    CameraCharacteristics characteristics = manager.getCameraCharacteristics(id);
                    if (characteristics.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL)
                            != CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY) {
                        allCamerasAreLegacy = false;
                        break;
                    }
                }
                if (allCamerasAreLegacy) {
                    showToast(R.string.all_legacy_devices);
                    getPassButton().setEnabled(false);
                }
            } catch (CameraAccessException e) {
                Toast.makeText(ItsTestActivity.this,
                        "Received error from camera service while checking device capabilities: "
                                + e, Toast.LENGTH_SHORT).show();
            }
            IntentFilter filter = new IntentFilter(ACTION_ITS_RESULT);
            registerReceiver(mSuccessReceiver, filter);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(mSuccessReceiver);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        setContentView(R.layout.its_main);
        setInfoResources(R.string.camera_its_test, R.string.camera_its_test_info, -1);
        setPassFailButtonClickListeners();
        getPassButton().setEnabled(false);
    }

    private void showToast(int messageId) {
        Toast.makeText(ItsTestActivity.this, messageId, Toast.LENGTH_SHORT).show();
    }

}
