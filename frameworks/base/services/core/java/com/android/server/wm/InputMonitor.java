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
import android.content.Context;
import android.content.pm.PackageManager;
import android.provider.Settings;

import com.android.server.input.InputManagerService;
import com.android.server.input.InputApplicationHandle;
import com.android.server.input.InputWindowHandle;

import android.app.ActivityManager;
import android.app.ActivityManagerNative;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;

import android.graphics.Rect;
import android.graphics.Point;
import android.graphics.Region;

import android.os.RemoteException;
import android.util.Log;
import android.util.Slog;
import android.view.Display;
import android.view.InputChannel;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.WindowManager;
import android.content.pm.ResolveInfo;
import android.content.ComponentName;
import java.util.ArrayList;
import java.util.List;
import android.app.IActivityManager;
import java.util.ArrayList;
import java.util.Arrays;
import android.view.WindowManagerPolicy;
import static android.view.WindowManager.LayoutParams.FIRST_APPLICATION_WINDOW;
import static android.view.WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN;
import static android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN;
import static android.view.WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
import static android.view.WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR;
import static android.view.WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;
import static android.view.WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED;
import static android.view.WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD;
import static android.view.WindowManager.LayoutParams.FLAG_ALLOW_LOCK_WHILE_SCREEN_ON;
import static android.view.WindowManager.LayoutParams.SOFT_INPUT_MASK_ADJUST;
import static android.view.WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE;
import static android.view.WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING;
import static android.view.WindowManager.LayoutParams.LAST_APPLICATION_WINDOW;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION_MEDIA;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION_MEDIA_OVERLAY;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION_PANEL;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION_SUB_PANEL;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION_ATTACHED_DIALOG;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION_STARTING;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION;
import static android.view.WindowManager.LayoutParams.FIRST_SUB_WINDOW;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION_MEDIA_OVERLAY;

import static android.view.WindowManager.LayoutParams.TYPE_DRAG;
import static android.view.WindowManager.LayoutParams.TYPE_DREAM;
import static android.view.WindowManager.LayoutParams.TYPE_HIDDEN_NAV_CONSUMER;
import static android.view.WindowManager.LayoutParams.TYPE_KEYGUARD;
import static android.view.WindowManager.LayoutParams.TYPE_KEYGUARD_DIALOG;
import static android.view.WindowManager.LayoutParams.TYPE_PHONE;
import static android.view.WindowManager.LayoutParams.TYPE_PRIORITY_PHONE;
import static android.view.WindowManager.LayoutParams.TYPE_SEARCH_BAR;
import static android.view.WindowManager.LayoutParams.TYPE_SECURE_SYSTEM_OVERLAY;
import static android.view.WindowManager.LayoutParams.TYPE_STATUS_BAR;
import static android.view.WindowManager.LayoutParams.TYPE_STATUS_BAR_PANEL;
import static android.view.WindowManager.LayoutParams.TYPE_STATUS_BAR_SUB_PANEL;
import static android.view.WindowManager.LayoutParams.TYPE_SYSTEM_DIALOG;
import static android.view.WindowManager.LayoutParams.TYPE_SYSTEM_ALERT;
import static android.view.WindowManager.LayoutParams.TYPE_SYSTEM_ERROR;
import static android.view.WindowManager.LayoutParams.TYPE_INPUT_METHOD;
import static android.view.WindowManager.LayoutParams.TYPE_INPUT_METHOD_DIALOG;
import static android.view.WindowManager.LayoutParams.TYPE_SYSTEM_OVERLAY;
import static android.view.WindowManager.LayoutParams.TYPE_TOAST;
import static android.view.WindowManager.LayoutParams.TYPE_VOLUME_OVERLAY;
import static android.view.WindowManager.LayoutParams.TYPE_WALLPAPER;
import static android.view.WindowManager.LayoutParams.TYPE_POINTER;
import static android.view.WindowManager.LayoutParams.TYPE_NAVIGATION_BAR;
import static android.view.WindowManager.LayoutParams.TYPE_NAVIGATION_BAR_PANEL;
import static android.view.WindowManager.LayoutParams.TYPE_BOOT_PROGRESS;

import com.android.multiwindow.wmservice.InputMonitorController;
import com.android.multiwindow.wmservice.DispatcherInterface;

final class InputMonitor implements InputManagerService.WindowManagerCallbacks ,DispatcherInterface {
	private static final String LOGTAG = "InputMonitor";
	private static boolean DEBUG_ZJY = false;
	private static boolean DEBUG_ZJY_MOTION = false;
	private void LOGD(String msg){
		if(DEBUG_ZJY){
			Log.d(LOGTAG,"QQQQQQQQQQQQQQQQQ:"+msg);
		}
	}
	private void LOGV(String msg){
		if(DEBUG_ZJY_MOTION){
			Log.d(LOGTAG,"QQQQQQQQQQQQQQQQQ:"+msg);
		}
	}
    private final WindowManagerService mService;
    
    // Current window with input focus for keys and other non-touch events.  May be null.
    private WindowState mInputFocus;
    
    // When true, prevents input dispatch from proceeding until set to false again.
    private boolean mInputDispatchFrozen;
    
    // When true, input dispatch proceeds normally.  Otherwise all events are dropped.
    // Initially false, so that input does not get dispatched until boot is finished at
    // which point the ActivityManager will enable dispatching.
    private boolean mInputDispatchEnabled;

    // When true, need to call updateInputWindowsLw().
    private boolean mUpdateInputWindowsNeeded = true;

    // Array of window handles to provide to the input dispatcher.
    private InputWindowHandle[] mInputWindowHandles;
    private int mInputWindowHandleCount;

    // Set to true when the first input device configuration change notification
    // is received to indicate that the input devices are ready.
    private final Object mInputDevicesReadyMonitor = new Object();
    private boolean mInputDevicesReady;
	// Set Focus on HOME window.
	private boolean mDontNeedFocusHome = true;

	Rect mTmpRect = new Rect();
    private InputMonitorController mInputMonitorController;

    public InputMonitor(WindowManagerService service) {
        mService = service;
	mInputMonitorController = new InputMonitorController(mService, mService.mContext, InputMonitor.this);
    }
    
    /* Notifies the window manager about a broken input channel.
     * 
     * Called by the InputManager.
     */
    @Override
    public void notifyInputChannelBroken(InputWindowHandle inputWindowHandle) {
        if (inputWindowHandle == null) {
            return;
        }

        synchronized (mService.mWindowMap) {
            WindowState windowState = (WindowState) inputWindowHandle.windowState;
            if (windowState != null) {
                Slog.i(WindowManagerService.TAG, "WINDOW DIED " + windowState);
                mService.removeWindowLocked(windowState.mSession, windowState);
            }
        }
    }
    
    /* Notifies the window manager about an application that is not responding.
     * Returns a new timeout to continue waiting in nanoseconds, or 0 to abort dispatch.
     * 
     * Called by the InputManager.
     */
    @Override
    public long notifyANR(InputApplicationHandle inputApplicationHandle,
            InputWindowHandle inputWindowHandle, String reason) {
        AppWindowToken appWindowToken = null;
        WindowState windowState = null;
        boolean aboveSystem = false;
        synchronized (mService.mWindowMap) {
            if (inputWindowHandle != null) {
                windowState = (WindowState) inputWindowHandle.windowState;
                if (windowState != null) {
                    appWindowToken = windowState.mAppToken;
                }
            }
            if (appWindowToken == null && inputApplicationHandle != null) {
                appWindowToken = (AppWindowToken)inputApplicationHandle.appWindowToken;
            }

            if (windowState != null) {
                Slog.i(WindowManagerService.TAG, "Input event dispatching timed out "
                        + "sending to " + windowState.mAttrs.getTitle()
                        + ".  Reason: " + reason);
                // Figure out whether this window is layered above system windows.
                // We need to do this here to help the activity manager know how to
                // layer its ANR dialog.
                int systemAlertLayer = mService.mPolicy.windowTypeToLayerLw(
                        WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
                aboveSystem = windowState.mBaseLayer > systemAlertLayer;
            } else if (appWindowToken != null) {
                Slog.i(WindowManagerService.TAG, "Input event dispatching timed out "
                        + "sending to application " + appWindowToken.stringName
                        + ".  Reason: " + reason);
            } else {
                Slog.i(WindowManagerService.TAG, "Input event dispatching timed out "
                        + ".  Reason: " + reason);
            }

            mService.saveANRStateLocked(appWindowToken, windowState, reason);
        }

        if (appWindowToken != null && appWindowToken.appToken != null) {
            try {
                // Notify the activity manager about the timeout and let it decide whether
                // to abort dispatching or keep waiting.
                boolean abort = appWindowToken.appToken.keyDispatchingTimedOut(reason);
                if (! abort) {
                    // The activity manager declined to abort dispatching.
                    // Wait a bit longer and timeout again later.
                    return appWindowToken.inputDispatchingTimeoutNanos;
                }
            } catch (RemoteException ex) {
            }
        } else if (windowState != null) {
            try {
                // Notify the activity manager about the timeout and let it decide whether
                // to abort dispatching or keep waiting.
                long timeout = ActivityManagerNative.getDefault().inputDispatchingTimedOut(
                        windowState.mSession.mPid, aboveSystem, reason);
                if (timeout >= 0) {
                    // The activity manager declined to abort dispatching.
                    // Wait a bit longer and timeout again later.
                    return timeout;
                }
            } catch (RemoteException ex) {
            }
        }
        return 0; // abort dispatching
    }

    private void addInputWindowHandleLw(final InputWindowHandle windowHandle) {
        if (mInputWindowHandles == null) {
            mInputWindowHandles = new InputWindowHandle[16];
        }
        if (mInputWindowHandleCount >= mInputWindowHandles.length) {
            mInputWindowHandles = Arrays.copyOf(mInputWindowHandles,
                    mInputWindowHandleCount * 2);
        }
        mInputWindowHandles[mInputWindowHandleCount++] = windowHandle;
    }

    private void addInputWindowHandleLw(final InputWindowHandle inputWindowHandle,
            final WindowState child, int flags, final int type, final boolean isVisible,
            final boolean hasFocus, final boolean hasWallpaper) {
        // Add a window to our list of input windows.
        inputWindowHandle.name = child.toString();
        if (false) Slog.d(WindowManagerService.TAG, ">>>>>> ENTERED addInputWindowsLw=================="+inputWindowHandle.name);
        final boolean modal = (flags & (WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE)) == 0;
        if (modal && child.mAppToken != null) {
            // Limit the outer touch to the activity stack region.
            flags |= WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL;
            child.getStackBounds(mTmpRect);
            inputWindowHandle.touchableRegion.set(mTmpRect);
        } else {
            // Not modal or full screen modal
            child.getTouchableRegion(inputWindowHandle.touchableRegion);
        }
        inputWindowHandle.layoutParamsFlags = flags;
        inputWindowHandle.layoutParamsType = type;
        inputWindowHandle.dispatchingTimeoutNanos = child.getInputDispatchingTimeoutNanos();
        inputWindowHandle.visible = isVisible;
        inputWindowHandle.canReceiveKeys = child.canReceiveKeys();
        inputWindowHandle.hasFocus = hasFocus;
        inputWindowHandle.hasWallpaper = hasWallpaper;
        inputWindowHandle.paused = child.mAppToken != null ? child.mAppToken.paused : false;
        inputWindowHandle.layer = child.mLayer;
        inputWindowHandle.ownerPid = child.mSession.mPid;
        inputWindowHandle.ownerUid = child.mSession.mUid;
        inputWindowHandle.inputFeatures = child.mAttrs.inputFeatures;

        final Rect frame = child.mFrame;
        inputWindowHandle.frameLeft = frame.left;
        inputWindowHandle.frameTop = frame.top;
        inputWindowHandle.frameRight = frame.right;
        inputWindowHandle.frameBottom = frame.bottom;

        if (child.mGlobalScale != 1) {
            // If we are scaling the window, input coordinates need
            // to be inversely scaled to map from what is on screen
            // to what is actually being touched in the UI.
            inputWindowHandle.scaleFactor = 1.0f/child.mGlobalScale;
        } else {
            inputWindowHandle.scaleFactor = 1;
        }


		inputWindowHandle.isHomeWindow = child.isHomeWindow();
        addInputWindowHandleLw(inputWindowHandle);
    }

    private void clearInputWindowHandlesLw() {
        while (mInputWindowHandleCount != 0) {
            mInputWindowHandles[--mInputWindowHandleCount] = null;
        }
    }

    public void setUpdateInputWindowsNeededLw() {
        mUpdateInputWindowsNeeded = true;
    }

    /* Updates the cached window information provided to the input dispatcher. */
    public void updateInputWindowsLw(boolean force) {
        if (!force && !mUpdateInputWindowsNeeded) {
            return;
        }
        mUpdateInputWindowsNeeded = false;

       if (false) Slog.d(WindowManagerService.TAG, ">>>>>> ENTERED updateInputWindowsLw");

        // Populate the input window list with information about all of the windows that
        // could potentially receive input.
        // As an optimization, we could try to prune the list of windows but this turns
        // out to be difficult because only the native code knows for sure which window
        // currently has touch focus.
        final WindowStateAnimator universeBackground = mService.mAnimator.mUniverseBackground;
        final int aboveUniverseLayer = mService.mAnimator.mAboveUniverseLayer;
        boolean addedUniverse = false;
        boolean disableWallpaperTouchEvents = false;

        // If there's a drag in flight, provide a pseudowindow to catch drag input
        final boolean inDrag = (mService.mDragState != null);
        if (inDrag) {
            if (WindowManagerService.DEBUG_DRAG) {
                Log.d(WindowManagerService.TAG, "Inserting drag window");
            }
            final InputWindowHandle dragWindowHandle = mService.mDragState.mDragWindowHandle;
            if (dragWindowHandle != null) {
                addInputWindowHandleLw(dragWindowHandle);
            } else {
                Slog.w(WindowManagerService.TAG, "Drag is in progress but there is no "
                        + "drag window handle.");
            }
        }

        final int NFW = mService.mFakeWindows.size();
        for (int i = 0; i < NFW; i++) {
            addInputWindowHandleLw(mService.mFakeWindows.get(i).mWindowHandle);
        }

        // Add all windows on the default display.
        final int numDisplays = mService.mDisplayContents.size();
        for (int displayNdx = 0; displayNdx < numDisplays; ++displayNdx) {
            WindowList windows = mService.mDisplayContents.valueAt(displayNdx).getWindowList();
            for (int winNdx = windows.size() - 1; winNdx >= 0; --winNdx) {
                final WindowState child = windows.get(winNdx);
                final InputChannel inputChannel = child.mInputChannel;
                final InputWindowHandle inputWindowHandle = child.mInputWindowHandle;
                if (inputChannel == null || inputWindowHandle == null || child.mRemoved) {
                    // Skip this window because it cannot possibly receive input.
                    continue;
                }

                final int flags = child.mAttrs.flags;
                final int privateFlags = child.mAttrs.privateFlags;
                final int type = child.mAttrs.type;

                final boolean hasFocus = (child == mInputFocus);
                final boolean isVisible = child.isVisibleLw();
                if ((privateFlags
                        & WindowManager.LayoutParams.PRIVATE_FLAG_DISABLE_WALLPAPER_TOUCH_EVENTS)
                            != 0) {
                    disableWallpaperTouchEvents = true;
                }
                final boolean hasWallpaper = (child == mService.mWallpaperTarget)
                        && (privateFlags & WindowManager.LayoutParams.PRIVATE_FLAG_KEYGUARD) == 0
                        && !disableWallpaperTouchEvents;
                final boolean onDefaultDisplay = (child.getDisplayId() == Display.DEFAULT_DISPLAY);

                // If there's a drag in progress and 'child' is a potential drop target,
                // make sure it's been told about the drag
                if (inDrag && isVisible && onDefaultDisplay) {
                    mService.mDragState.sendDragStartedIfNeededLw(child);
                }

                if (universeBackground != null && !addedUniverse
                        && child.mBaseLayer < aboveUniverseLayer && onDefaultDisplay) {
                    final WindowState u = universeBackground.mWin;
                    if (u.mInputChannel != null && u.mInputWindowHandle != null) {
                        addInputWindowHandleLw(u.mInputWindowHandle, u, u.mAttrs.flags,
                                u.mAttrs.type, true, u == mInputFocus, false);
                    }
                    addedUniverse = true;
                }

                if (child.mWinAnimator != universeBackground) {
                    addInputWindowHandleLw(inputWindowHandle, child, flags, type, isVisible,
                            hasFocus, hasWallpaper);
                }
            }
        }

        // Send windows to native code.
        mService.mInputManager.setInputWindows(mInputWindowHandles);
		mService.mInputManager.setDontFocusedHome(mDontNeedFocusHome);
        // Clear the list in preparation for the next round.
        clearInputWindowHandlesLw();

       if (false) Slog.d(WindowManagerService.TAG, "<<<<<<< EXITED updateInputWindowsLw");
    }

    /* Notifies that the input device configuration has changed. */
    @Override
    public void notifyConfigurationChanged() {
        mService.sendNewConfiguration();

        synchronized (mInputDevicesReadyMonitor) {
            if (!mInputDevicesReady) {
                mInputDevicesReady = true;
                mInputDevicesReadyMonitor.notifyAll();
            }
        }
    }

    /* Waits until the built-in input devices have been configured. */
    public boolean waitForInputDevicesReady(long timeoutMillis) {
        synchronized (mInputDevicesReadyMonitor) {
            if (!mInputDevicesReady) {
                try {
                    mInputDevicesReadyMonitor.wait(timeoutMillis);
                } catch (InterruptedException ex) {
                }
            }
            return mInputDevicesReady;
        }
    }

    /* Notifies that the lid switch changed state. */
    @Override
    public void notifyLidSwitchChanged(long whenNanos, boolean lidOpen) {
        mService.mPolicy.notifyLidSwitchChanged(whenNanos, lidOpen);
    }

    /* Notifies that the camera lens cover state has changed. */
    @Override
    public void notifyCameraLensCoverSwitchChanged(long whenNanos, boolean lensCovered) {
        mService.mPolicy.notifyCameraLensCoverSwitchChanged(whenNanos, lensCovered);
    }

    /* Provides an opportunity for the window manager policy to intercept early key
     * processing as soon as the key has been read from the device. */
    @Override
    public int interceptKeyBeforeQueueing(KeyEvent event, int policyFlags) {
        return mService.mPolicy.interceptKeyBeforeQueueing(event, policyFlags);
    }

    /* Provides an opportunity for the window manager policy to intercept early motion event
     * processing when the device is in a non-interactive state since these events are normally
     * dropped. */
    @Override
    public int interceptMotionBeforeQueueingNonInteractive(long whenNanos, int policyFlags) {
        return mService.mPolicy.interceptMotionBeforeQueueingNonInteractive(
                whenNanos, policyFlags);
    }

    /* Provides an opportunity for the window manager policy to process a key before
     * ordinary dispatch. */
    @Override
    public long interceptKeyBeforeDispatching(
            InputWindowHandle focus, KeyEvent event, int policyFlags) {
        WindowState windowState = focus != null ? (WindowState) focus.windowState : null;
        return mService.mPolicy.interceptKeyBeforeDispatching(windowState, event, policyFlags);
    }
private boolean modeChange = false;
private WindowState mCurFocusWindowState = null;
private boolean validWindowState(WindowState win){
	boolean isValid = false;
	if(win != null && "com.android.systemui/com.android.systemui.recent.RecentsActivity".equals(win.getAttrs().getTitle())){
        isValid = false;
	}else if(mService.isHomeWindow(win)){
		isValid = false;
	}else if((win.getAttrs().type == WindowManager.LayoutParams.TYPE_BASE_APPLICATION||win.getAttrs().type == WindowManager.LayoutParams.TYPE_APPLICATION) &&
			((win.getAttrs().width == -1 && win.getAttrs().height == -1)||(win.getAttrs().align == WindowManagerPolicy.WINDOW_ALIGN_RIGHT))){
			//google map run in here
			//live wallpaper 
		isValid = true;
	}
	return isValid;
}


	/* Provides an opportunity for the window manager policy to process a motion before
     * ordinary dispatch. 
     * return 0: MotionEntry::INTERCEPT_MOTION_RESULT_CONTINUE
     * return -1(<0): MotionEntry::INTERCEPT_MOTION_RESULT_SKIP
     * return other value:MotionEntry::INTERCEPT_MOTION_RESULT_TRY_AGAIN_LATER
     */
     int x1=0 ,y1=0,up_x,up_y,mov_x,mov_y,temp_x=0;
     Rect mRect = null;
     boolean isSideSlip = false;
     public long interceptMotionBeforeDispatching(
     		InputWindowHandle focus,MotionEvent event,int policyFlags){
     		int action = event.getAction();	
		int screenWidth = mService.getDefaultDisplayInfoLocked().logicalWidth;
		if (mService.mCurConfiguration.dualscreenflag == Configuration.ENABLE_DUAL_SCREEN && action == MotionEvent.ACTION_HOVER_MOVE) {
			x1 = (int)event.getX();
			y1 = (int)event.getY();
			if (x1 + 15 > screenWidth) {
				if(!mService.isWorked("com.android.Listappinfo.ManderService")){
					LOGV("start com.android.Listappinfo.ManderService");
					Intent intent = new Intent();
					intent.setClassName("com.android.Listappinfo", "com.android.Listappinfo.ManderService");
					mService.mContext.startService(intent);
					return -1;
				}
			}
		}
     	 if(mService.mCurConfiguration.multiwindowflag == Configuration.DISABLE_MULTI_WINDOW){
			return 0;
		}
			  
		WindowState windowState = focus != null ? (WindowState) focus.windowState : null;		
		boolean used = mInputMonitorController.isMultiWindowUsed(mService.mContext);
		long result = 0;
		int policy = WindowManagerPolicy.MOTION_PASS_TO_USER;

		//LOGV(windowState.getAttrs()+"MotionEvent event="+screenWidth+"    event"+(windowState.getAttrs()));
		if(windowState != null){
			if(validWindowState(windowState)){
				mCurFocusWindowState = windowState;
				LOGV("mCurFocusWindow="+mCurFocusWindowState);
			}else{
				mCurFocusWindowState = null;
			}
		mService.applyXTrac(windowState, action, event);

                if(mService.isMultiWindowMode() || (windowState.getAttrs() != null && windowState.getAttrs().align == WindowManagerPolicy.WINDOW_ALIGN_RIGHT
							&&windowState.getAttrs().width != screenWidth/2)){
			if(event.getPointerCount() == 1 && windowState != null){
			    WindowState ws = null;
			    if(windowState.mAppWindowState != null){
			       ws = windowState.mAppWindowState;
			    }else if(windowState.mAttachedWindow!=null){
			       ws = windowState.mAttachedWindow;
			    }else{
			       ws = windowState;
			    }
                        //action = action & MotionEvent.ACTION_MASK;
			if(action== MotionEvent.ACTION_DOWN){
			    x1 = (int)event.getX();
			    y1 = (int)event.getY();
                            mov_x = 0;
                            mov_y = 0;
                            int x_width = (int)(ws.mPosX)+(int)(ws.mHScale*ws.mVisibleFrame.width());	
                           if(x1 >ws.mPosX && x1 < x_width && y1 < ws.mPosY+100*ws.mActualScale&& y1> windowState.mPosY){
			     //LOGV(ws.mPosY+"down to position=ooo=22=============="+x_width);
                             isSideSlip = true;
			    }
			    //LOGV(ws.mPosY+"down to position=ooo==============="+x_width+" X1 "+x1+" Y1 "+y1+"windowState.mPosX "+ws.mPosX);
			}else if(action ==MotionEvent.ACTION_MOVE){
					   
                            int posX =ws.mPosX;
			    int posY = ws.mPosY;
	                    LOGV(mov_y+"move to position =   ooo===============mov_x"+mov_x+",posX:"+posX+",posY"+posY+" x"+ isSideSlip);
                        if(mov_x == 0 && mov_y ==0) {
			    mov_x = (int)event.getX();
			    mov_y = (int)event.getY();
 			    return 0;
			}
                        if(!isSideSlip) return 0;
			float barheight = mService.getStatusBarHeight();
			float move_rel = posY + (int)event.getY()-mov_y;
			if (!((move_rel + ws.mSfOffsetY) > barheight)) {
			   move_rel = barheight - ws.mSfOffsetY;
			}
			 mService.applyPositionForMultiWindow(ws,posX+(int)event.getX()- mov_x,posY+ (int)event.getY()-mov_y);
			 mov_x = (int)event.getX();
			 mov_y = (int)event.getY();
			 return 0;
		     }else if(action ==MotionEvent.ACTION_UP){
			  up_x = (int)event.getX();
			  up_y = (int)event.getY();
			 if (isSideSlip && ((up_x > (x1 + 50)) || (up_x < (x1 - 30)))) {
				mService.applySizeForMultiWindow(ws);
			 }
                       	  mov_x = 0;
                       	  mov_y = 0;
                          isSideSlip = false;
			 return 0;
		      }
 	          }
	    }
			
			//don't switch foucs app when current foucs window is not base_application
			//contains(dialog,NotificationPanel,RecentPanel,SearchPanel,inputPanel)
			//the second condition for activity type with dialog theme(ig: choose Wallpaper,
			// choose an app for intent)
			// google map activity type==2 and height == width == -1
			//
			if((windowState.getAttrs().type != WindowManager.LayoutParams.TYPE_BASE_APPLICATION||
				(windowState.getAttrs().flags&WindowManager.LayoutParams.FLAG_DIM_BEHIND)!=0) && 
					(windowState.getAttrs().width!=-1 || windowState.getAttrs().height != -1)){
					LOGD("focus window have flag_dim_behind flag jut return ");
					 if (!mDontNeedFocusHome) {
						mDontNeedFocusHome = true;
						windowState.mDisplayContent.layoutNeeded = true;
						mService.performLayoutAndPlaceSurfacesLocked();
					}
					 //will reset the multiwindow mode when these windows fouced 
				mInputMonitorController.setMultiWindowUsed(mService.mContext, true);
				return 0;

			}
			//find the focus app.
			int touchX = (int)event.getRawX();
			int touchY = (int)event.getRawY();
			Rect df = windowState.getSurfaceFrameLw();
			//first judge the launcher app
			LOGD("df="+df.toString()+"title="+windowState.getAttrs().getTitle().toString()+
				"touch("+touchX+","+touchY+")");
				//now find the window which will get the focus.				
				WindowState matchFocus = null;
				boolean isMatch = false;
				if(df.contains(touchX, touchY)) {
					LOGD("we don't need to match again");
				} else if (action== MotionEvent.ACTION_DOWN){
				WindowList windows = mService.getDefaultDisplayContentLocked().getWindowList();
				for(int i=windows.size()-1;i>=0;i--){
					matchFocus = windows.get(i);
					WindowManager.LayoutParams lp = matchFocus.getAttrs();
					if((lp.flags & WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE)!=0){
						continue;
					}
                    if (matchFocus.mWinAnimator.mSurfaceShown) {
                        Rect frame = matchFocus.getSurfaceFrameLw();
						if(matchFocus.mTouchableInsets != 0){
							Region r = new Region();
							matchFocus.getTouchableRegion(r);
							r.getBounds(frame);
						}
						float posX = matchFocus.mWinAnimator.mSurfaceX;
						float posY = matchFocus.mWinAnimator.mSurfaceY;
						float posW = matchFocus.mWinAnimator.mSurfaceW;
						float posH = matchFocus.mWinAnimator.mSurfaceH;
						boolean input = false;
						if (matchFocus.mAppWindowState != null)
							input = matchFocus.mAppWindowState.mIsImWindow;//mAttrs.type == TYPE_INPUT_METHOD;
						if(frame.contains(touchX, touchY) || 
						(input && !(touchX < posX) && !(touchX > (posX + posW)) &&
							!(touchY < posY) && !(touchY > (posY + posH)))){
							if(mService.isHomeWindow(matchFocus)){
								policy = WindowManagerPolicy.MOTION_PASS_TO_HOME;
							}else if(matchFocus != windowState){
									isMatch = true;	
							}
							break;
						}
					}
				}
				}
		
				if(isMatch){
					mInputMonitorController.switchFocusWindow(mService.mContext, matchFocus);
				}
		}else{
			LOGD("focus window is null i do not know how to do ");
			return 0;
		}
		//LOGD("policy="+policy);
		if(policy == WindowManagerPolicy.MOTION_PASS_TO_HOME){
			mDontNeedFocusHome = false;
			mService.mInputManager.setDontFocusedHome(mDontNeedFocusHome);
			return 0;
		}else if(policy == WindowManagerPolicy.MOTION_PASS_TO_USER){
		 if (!mDontNeedFocusHome) {
                    mDontNeedFocusHome = true;
                    windowState.mDisplayContent.layoutNeeded = true;
                    mService.performLayoutAndPlaceSurfacesLocked();
                }
			return 0;
		}
		return 0;
	}

private boolean maybeModeChange(Point p1, Point p2, Point p3){
	LOGV("maybeModeChange mCurFocuswindow="+mCurFocusWindowState);
	if(mCurFocusWindowState == null){
		return false;
	}
	boolean modeChange = false;
	Rect bounds = mCurFocusWindowState.getSurfaceFrameLw();
	boolean c1 = bounds.contains(p1.x, p1.y);
	boolean c2 = bounds.contains(p2.x, p2.y);
	boolean c3 = bounds.contains(p3.x, p3.y);
	if(c1 != c2 ||(c1 && c2 && c3)){
		modeChange = true;
	}
	LOGV("maybeModeChange p1="+p1.toString()+" p2="+p2.toString()+" modeChange="+modeChange);
	return modeChange;
	
}

@Override
public boolean maybeModeChangeInterface(Point p1, Point p2, Point p3) {
	// TODO Auto-generated method stub
	return maybeModeChange(p1, p2, p3);
}

@Override
public boolean isWorkedInterface(String name) {
	// TODO Auto-generated method stub
	return false;
}

@Override
public boolean isAppTokenNull(WindowState windowstate) {
	// TODO Auto-generated method stub
	return windowstate.mAppToken!=null && windowstate.mAppToken.appToken!=null;
}

@Override
public int getAppTokenTaskId(WindowState windowstate){
	// TODO Auto-generated method stub
	int taskid = -100;
	try {
		taskid = ActivityManagerNative.getDefault().getTaskForActivity(windowstate.mAppToken.appToken.asBinder(),false);
	} catch (RemoteException e){		
		LOGV("get apptoken remoteException");
	}
	return taskid;
}




    /* Provides an opportunity for the window manager policy to process a key that
     * the application did not handle. */
    @Override
    public KeyEvent dispatchUnhandledKey(
            InputWindowHandle focus, KeyEvent event, int policyFlags) {
        WindowState windowState = focus != null ? (WindowState) focus.windowState : null;
        return mService.mPolicy.dispatchUnhandledKey(windowState, event, policyFlags);
    }

    /* Callback to get pointer layer. */
    @Override
    public int getPointerLayer() {
        return mService.mPolicy.windowTypeToLayerLw(WindowManager.LayoutParams.TYPE_POINTER)
                * WindowManagerService.TYPE_LAYER_MULTIPLIER
                + WindowManagerService.TYPE_LAYER_OFFSET;
    }

    /* Called when the current input focus changes.
     * Layer assignment is assumed to be complete by the time this is called.
     */
    public void setInputFocusLw(WindowState newWindow, boolean updateInputWindows) {
        if (WindowManagerService.DEBUG_FOCUS_LIGHT || WindowManagerService.DEBUG_INPUT) {
            Slog.d(WindowManagerService.TAG, "Input focus has changed to " + newWindow);
        }

              Log.d(WindowManagerService.TAG, newWindow+"  ==========setInputFocusLw ==========  =======");
        if (newWindow != mInputFocus) {
            if (newWindow != null && newWindow.canReceiveKeys()) {
                // Displaying a window implicitly causes dispatching to be unpaused.
                // This is to protect against bugs if someone pauses dispatching but
                // forgets to resume.
                newWindow.mToken.paused = false;
            }

            mInputFocus = newWindow;
            setUpdateInputWindowsNeededLw();

            if (updateInputWindows) {
                updateInputWindowsLw(false /*force*/);
            }
        }
    }

    public void setFocusedAppLw(AppWindowToken newApp) {
        // Focused app has changed.
        if (newApp == null) {
            mService.mInputManager.setFocusedApplication(null);
        } else {
            final InputApplicationHandle handle = newApp.mInputApplicationHandle;
            handle.name = newApp.toString();
            handle.dispatchingTimeoutNanos = newApp.inputDispatchingTimeoutNanos;

            mService.mInputManager.setFocusedApplication(handle);
        }
    }

    public void pauseDispatchingLw(WindowToken window) {
        if (! window.paused) {
            if (WindowManagerService.DEBUG_INPUT) {
                Slog.v(WindowManagerService.TAG, "Pausing WindowToken " + window);
            }
            
            window.paused = true;
            updateInputWindowsLw(true /*force*/);
        }
    }
    
    public void resumeDispatchingLw(WindowToken window) {
        if (window.paused) {
            if (WindowManagerService.DEBUG_INPUT) {
                Slog.v(WindowManagerService.TAG, "Resuming WindowToken " + window);
            }
            
            window.paused = false;
            updateInputWindowsLw(true /*force*/);
        }
    }
    
    public void freezeInputDispatchingLw() {
        if (! mInputDispatchFrozen) {
            if (WindowManagerService.DEBUG_INPUT) {
                Slog.v(WindowManagerService.TAG, "Freezing input dispatching");
            }
            
            mInputDispatchFrozen = true;
            updateInputDispatchModeLw();
        }
    }
    
    public void thawInputDispatchingLw() {
        if (mInputDispatchFrozen) {
            if (WindowManagerService.DEBUG_INPUT) {
                Slog.v(WindowManagerService.TAG, "Thawing input dispatching");
            }
            
            mInputDispatchFrozen = false;
            updateInputDispatchModeLw();
        }
    }
    
    public void setEventDispatchingLw(boolean enabled) {
        if (mInputDispatchEnabled != enabled) {
            if (WindowManagerService.DEBUG_INPUT) {
                Slog.v(WindowManagerService.TAG, "Setting event dispatching to " + enabled);
            }
            
            mInputDispatchEnabled = enabled;
            updateInputDispatchModeLw();
        }
    }
    
    private void updateInputDispatchModeLw() {
        mService.mInputManager.setInputDispatchMode(mInputDispatchEnabled, mInputDispatchFrozen);
    }
}
