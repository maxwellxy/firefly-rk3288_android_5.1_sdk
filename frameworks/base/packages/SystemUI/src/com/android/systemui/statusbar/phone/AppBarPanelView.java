/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.systemui.statusbar.phone;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.Slog;
import android.view.MotionEvent;
import android.view.View;

import com.android.systemui.R;
import com.android.systemui.statusbar.GestureRecorder;

public class AppBarPanelView extends PanelView{
	Drawable mHandleBar;
    int mHandleBarHeight;
    View mHandleView;
    int mFingers;
    PhoneStatusBar mStatusBar;
    boolean mOkToFlip;
	HSbar mHSbar;
    public AppBarPanelView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }
	 public void setStatusBar(PhoneStatusBar bar) {
        mStatusBar = bar;
    }

		    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        Resources resources = getContext().getResources();
        mHandleBar = resources.getDrawable(R.drawable.status_bar_close);
        mHandleBarHeight = resources.getDimensionPixelSize(R.dimen.close_handle_height);
        mHandleView = findViewById(R.id.handle);
		mHSbar = (HSbar)findViewById(R.id.app_bar);
        setContentDescription(resources.getString(R.string.accessibility_desc_app_bar));
    }
	public void setBar(PanelBar bar){
		super.setBar(bar);
		if(mHSbar != null){
			mHSbar.setBar(bar);
		}
	}
	    @Override
    public void fling(float vel, boolean always) {
        GestureRecorder gr = ((PhoneStatusBarView) mBar).mBar.getGestureRecorder();
        if (gr != null) {
            gr.tag(
                "fling " + ((vel > 0) ? "open" : "closed"),
                "notifications,v=" + vel);
        }
        super.fling(vel, always);
    }
    // We draw the handle ourselves so that it's always glued to the bottom of the window.
    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        if (changed) {
            final int pl = getPaddingLeft();
            final int pr = getPaddingRight();
            mHandleBar.setBounds(pl, 0, getWidth() - pr, (int) mHandleBarHeight);
        }
    }

	    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);
        final int off = (int) (getHeight() - mHandleBarHeight - getPaddingBottom());
        canvas.translate(0, off);
        mHandleBar.setState(mHandleView.getDrawableState());
        mHandleBar.draw(canvas);
        canvas.translate(0, -off);
    }

protected  boolean hasConflictingGestures(){ return false;}
 protected  boolean isInContentBounds(float x, float y){return false;}
 protected  boolean isTrackingBlocked(){return false;}

    protected  void setOverExpansion(float overExpansion, boolean isPixels){}

    protected  void onHeightUpdated(float expandedHeight){}

    protected  float getOverExpansionAmount(){return 0f;}

    protected  float getOverExpansionPixels(){return 0f;}
    protected  int getMaxPanelHeight(){return 0;}
    protected  void onEdgeClicked(boolean right){}

    protected  boolean isDozing(){return false;}
   public  void resetViews(){}

    protected  float getPeekHeight(){return 0f;}

    protected  float getCannedFlingDurationFactor(){return 0f;}

    /**
     * @return whether "Clear all" button will be visible when the panel is fully expanded
     */
    protected  boolean fullyExpandedClearAllVisible(){return false;}

    protected  boolean isClearAllVisible(){return false;}

    /**
     * @return the height of the clear all button, in pixels
     */
    protected  int getClearAllHeight(){return 0;}
} 

