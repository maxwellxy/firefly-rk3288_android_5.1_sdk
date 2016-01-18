/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.server.wm;


import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.Region;
import android.view.Display;
import android.view.Surface;
import android.view.SurfaceSession;
import android.view.SurfaceControl;
import android.view.Surface.OutOfResourcesException;
import android.util.Log;

class SurfaceViewBackWindow {
    private final SurfaceControl mSurfaceControl;
    private final Surface mSurface = new Surface();
    int mLastDW;
    int mLastDH;
    boolean mDrawNeeded;
    final int mThickness = 20;
	public int mLayer = 0;

    public SurfaceViewBackWindow(Display display, SurfaceSession session) {
		SurfaceControl ctrl = null;
        try {
            ctrl = new SurfaceControl(session, "SurfaceViewBackWindow",
                1, 1, PixelFormat.TRANSLUCENT, SurfaceControl.HIDDEN);
            ctrl.setLayerStack(display.getLayerStack());
            ctrl.setPosition(0, 0);
            ctrl.show();
            mSurface.copyFrom(ctrl);
        } catch (OutOfResourcesException e) {
        }
        mSurfaceControl = ctrl;
        mDrawNeeded = true;
    }

    private void drawIfNeeded() {
        if (!mDrawNeeded) {
            return;
        }
        mDrawNeeded = false;
        final int dw = mLastDW;
        final int dh = mLastDH;
		Log.v("SurfaceViewBackWindow","---------drawIfNeeded------------dw:"+dw+",dh:"+dh);
        Rect dirty = new Rect(0, 0, dw, dh);
        Canvas c = null;
        try {
            c = mSurface.lockCanvas(dirty);
        } catch (IllegalArgumentException e) {
        } catch (Surface.OutOfResourcesException e) {
        }
        if (c == null) {
            return;
        }

        c.clipRect(new Rect(0, 0, dw, dh),  Region.Op.REPLACE);
        c.drawColor(0xFF000000);	//set the white color
        mLastDW = c.getWidth();
		mLastDH = c.getHeight();

        mSurface.unlockCanvasAndPost(c);
    }

    // Note: caller responsible for being inside
    // Surface.openTransaction() / closeTransaction()
    public void setVisibility(boolean on) {
        if (mSurfaceControl == null) {
            return;
        }
        drawIfNeeded();
        if (on) {
            mSurfaceControl.show();
        } else {
            mSurfaceControl.hide();
        }
    }

    void positionSurface(int posX,int posY,int dw, int dh) {
        if (mLastDW == dw && mLastDH == dh) {
            return;
        }
		Log.v("SurfaceViewBackWindow","---------------------posX:"+posX+",posY:"+posY+",dw:"+dw+",dh:"+dh);
        mLastDW = dw;
        mLastDH = dh;
		if(dw == 0 || dh == 0){
			setVisibility(false);
		}else{
			setVisibility(true);
			mSurfaceControl.setPosition(posX,posY);
       	 	mSurfaceControl.setSize(dw, dh);
		}
        mDrawNeeded = true;
    }

    void SetLayer(int layer){
        if(mSurfaceControl != null){
			Log.v("SurfaceViewBackWindow","---------------------layer:"+layer);
			mLayer = layer;
            mSurfaceControl.setLayer(layer);
        }
    }

	void destroy(){
		SetLayer(0);
		setVisibility(false);
		if(mSurfaceControl != null){
			mSurfaceControl.destroy();
		}
		if(mSurface != null){
			mSurface.destroy();
		}
	}

}
