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

package com.android.systemui.statusbar.phone;


import static android.app.StatusBarManager.NAVIGATION_HINT_BACK_ALT;
import static android.app.StatusBarManager.NAVIGATION_HINT_IME_SHOWN;
import static android.app.StatusBarManager.WINDOW_STATE_HIDDEN;
import static android.app.StatusBarManager.WINDOW_STATE_SHOWING;
import static android.app.StatusBarManager.windowStateToString;
import static com.android.systemui.statusbar.phone.BarTransitions.MODE_LIGHTS_OUT;
import static com.android.systemui.statusbar.phone.BarTransitions.MODE_LIGHTS_OUT_TRANSPARENT;
import static com.android.systemui.statusbar.phone.BarTransitions.MODE_OPAQUE;
import static com.android.systemui.statusbar.phone.BarTransitions.MODE_SEMI_TRANSPARENT;
import static com.android.systemui.statusbar.phone.BarTransitions.MODE_TRANSLUCENT;
import static com.android.systemui.statusbar.phone.BarTransitions.MODE_TRANSPARENT;
import static com.android.systemui.statusbar.phone.BarTransitions.MODE_WARNING;
import com.android.systemui.statusbar.phone.PhoneCards.CardInfo;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.TimeInterpolator;
import android.annotation.NonNull;
import android.app.ActivityManager;
import android.app.ActivityManagerNative;
import android.app.IActivityManager;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.StatusBarManager;
import android.content.BroadcastReceiver;
import android.content.ComponentCallbacks2;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.inputmethodservice.InputMethodService;
import android.media.AudioAttributes;
import android.media.MediaMetadata;
import android.media.session.MediaController;
import android.media.session.MediaSession;
import android.media.session.MediaSessionManager;
import android.media.session.PlaybackState;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Message;
import android.os.PowerManager;
import android.os.Process;
import android.os.RemoteException;
import android.os.SystemClock;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.service.notification.NotificationListenerService;
import android.service.notification.NotificationListenerService.RankingMap;
import android.service.notification.StatusBarNotification;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.DisplayMetrics;
import android.util.EventLog;
import android.util.Log;
import android.view.Display;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.HardwareCanvas;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewGroup;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup.LayoutParams;
import android.view.ViewPropertyAnimator;
import android.view.ViewStub;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import android.view.WindowManagerGlobal;
import android.view.accessibility.AccessibilityEvent;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Animation;
import android.view.animation.AnimationSet;
import android.view.animation.AnimationUtils;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.Interpolator;
import android.view.animation.LinearInterpolator;
import android.view.animation.PathInterpolator;
import android.view.animation.ScaleAnimation;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.os.SystemProperties;
import android.content.res.Configuration;
import com.android.internal.statusbar.StatusBarIcon;
import com.android.keyguard.KeyguardHostView.OnDismissAction;
import com.android.keyguard.ViewMediatorCallback;
import com.android.systemui.BatteryMeterView;
import com.android.systemui.DemoMode;
import com.android.systemui.EventLogConstants;
import com.android.systemui.EventLogTags;
import com.android.systemui.FontSizeUtils;
import com.android.systemui.R;
import com.android.systemui.doze.DozeHost;
import com.android.systemui.doze.DozeLog;
import com.android.systemui.keyguard.KeyguardViewMediator;
import com.android.systemui.qs.QSPanel;
import com.android.systemui.recent.ScreenPinningRequest;
import com.android.systemui.statusbar.ActivatableNotificationView;
import com.android.systemui.statusbar.BackDropView;
import com.android.systemui.statusbar.BaseStatusBar;
import com.android.systemui.statusbar.CommandQueue;
import com.android.systemui.statusbar.DismissView;
import com.android.systemui.statusbar.DragDownHelper;
import com.android.systemui.statusbar.EmptyShadeView;
import com.android.systemui.statusbar.ExpandableNotificationRow;
import com.android.systemui.statusbar.GestureRecorder;
import com.android.systemui.statusbar.KeyguardIndicationController;
import com.android.systemui.statusbar.NotificationData;
import com.android.systemui.statusbar.NotificationData.Entry;
import com.android.systemui.statusbar.NotificationOverflowContainer;
import com.android.systemui.statusbar.ScrimView;
import com.android.systemui.statusbar.SignalClusterView;
import com.android.systemui.statusbar.SpeedBumpView;
import com.android.systemui.statusbar.StatusBarIconView;
import com.android.systemui.statusbar.StatusBarState;
import com.android.systemui.statusbar.phone.UnlockMethodCache.OnUnlockMethodChangedListener;
import com.android.systemui.statusbar.policy.AccessibilityController;
import com.android.systemui.statusbar.policy.BatteryController;
import com.android.systemui.statusbar.policy.BatteryController.BatteryStateChangeCallback;
import com.android.systemui.statusbar.policy.BluetoothControllerImpl;
import com.android.systemui.statusbar.policy.BrightnessMirrorController;
import com.android.systemui.statusbar.policy.CastControllerImpl;
import com.android.systemui.statusbar.policy.FlashlightController;
import com.android.systemui.statusbar.policy.HeadsUpNotificationView;
import com.android.systemui.statusbar.policy.HotspotControllerImpl;
import com.android.systemui.statusbar.policy.KeyButtonView;
import com.android.systemui.statusbar.policy.KeyguardMonitor;
import com.android.systemui.statusbar.policy.KeyguardUserSwitcher;
import com.android.systemui.statusbar.policy.LocationControllerImpl;
import com.android.systemui.statusbar.policy.NetworkControllerImpl;
import com.android.systemui.statusbar.policy.NextAlarmController;
import com.android.systemui.statusbar.policy.PreviewInflater;
import com.android.systemui.statusbar.policy.RotationLockControllerImpl;
import com.android.systemui.statusbar.policy.SecurityControllerImpl;
import com.android.systemui.statusbar.policy.UserInfoController;
import com.android.systemui.statusbar.policy.UserSwitcherController;
import com.android.systemui.statusbar.policy.ZenModeController;
import com.android.systemui.statusbar.stack.NotificationStackScrollLayout;
import com.android.systemui.statusbar.stack.NotificationStackScrollLayout.OnChildLocationsChangedListener;
import com.android.systemui.statusbar.stack.StackScrollAlgorithm;
import com.android.systemui.statusbar.stack.StackScrollState.ViewState;
import com.android.systemui.volume.VolumeComponent;
import com.android.systemui.statusbar.circlemenu.FourScreenCircleMenuView;
import com.android.systemui.statusbar.circlemenu.FourScreenMenuWindow;
import com.android.systemui.statusbar.minwindow.MinWindow;
import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashMap;

import android.content.ServiceConnection;
import android.content.ComponentName;
import android.os.Messenger;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import java.util.Collection;
import java.util.Collections;
import java.io.File;
import java.util.List;
import android.widget.Toast;
import android.widget.ImageView.ScaleType;
import android.provider.Settings;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import java.util.Map;
import android.os.SystemProperties;
import android.content.res.Configuration;
import android.view.WindowManagerImpl;
import com.android.systemui.statusbar.circlemenu.CircleMenuView;
import com.android.systemui.statusbar.fourscreen.FourScreenBackWindow;
import java.util.HashMap; 

import android.app.ActivityManager;
import android.app.ActivityManagerNative;
import android.app.ActivityManager.RunningAppProcessInfo;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.SimpleAdapter; 
import android.widget.AdapterView.OnItemLongClickListener; 
import android.widget.SimpleAdapter.ViewBinder;
import android.widget.GridView;
import android.widget.HorizontalScrollView;
import android.content.pm.ApplicationInfo; 
 
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageItemInfo;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import android.app.ActivityManager.RunningServiceInfo;
import android.app.Dialog;
import android.app.AlertDialog;
import android.widget.ListView;
import android.widget.Button;
import android.view.InputDevice;
import android.app.AppGlobals;

import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.InputMethodInfo;
import android.content.DialogInterface;


import android.view.IWindowManager;
import android.view.WindowManagerGlobal;
import android.view.Surface;

public class PhoneStatusBar extends BaseStatusBar implements DemoMode,
        DragDownHelper.DragDownCallback, ActivityStarter, OnUnlockMethodChangedListener,CircleMenuView.BtnClickCallBack {
    static final String TAG = "PhoneStatusBar";
    public static final boolean DEBUG = BaseStatusBar.DEBUG;
	private static final boolean DEBUG_ZJY = false;
	private static final boolean DEBUG_ZJY_CONFIG = false;
	private static void LOGD(String msg){
		if(DEBUG_ZJY){
			Log.d(TAG, msg);
		}
	}

	public static void LOGR(String msg){
		if(DEBUG_ZJY_CONFIG){
			Log.d(TAG,msg);
		}
	}
    public static final boolean SPEW = false;
    public static final boolean DUMPTRUCK = true; // extra dumpsys info
    public static final boolean DEBUG_GESTURES = false;
    public static final boolean DEBUG_MEDIA = false;
    public static final boolean DEBUG_MEDIA_FAKE_ARTWORK = false;

    public static final boolean DEBUG_WINDOW_STATE = false;

    public static final boolean SETTINGS_DRAG_SHORTCUT = true;

    // additional instrumentation for testing purposes; intended to be left on during development
    public static final boolean CHATTY = DEBUG;

    public static final String ACTION_STATUSBAR_START
            = "com.android.internal.policy.statusbar.START";

    public static final boolean SHOW_LOCKSCREEN_MEDIA_ARTWORK = true;

    private static final int MSG_OPEN_NOTIFICATION_PANEL = 1000;
    private static final int MSG_CLOSE_PANELS = 1001;
    private static final int MSG_OPEN_SETTINGS_PANEL = 1002;
    private static final int MSG_LAUNCH_TRANSITION_TIMEOUT = 1003;
    // 1020-1040 reserved for BaseStatusBar

    // Time after we abort the launch transition.
    private static final long LAUNCH_TRANSITION_TIMEOUT_MS = 5000;

    private static final boolean CLOSE_PANEL_WHEN_EMPTIED = true;

    private static final int NOTIFICATION_PRIORITY_MULTIPLIER = 10; // see NotificationManagerService
    private static final int HIDE_ICONS_BELOW_SCORE = Notification.PRIORITY_LOW * NOTIFICATION_PRIORITY_MULTIPLIER;

    private static final int STATUS_OR_NAV_TRANSIENT =
            View.STATUS_BAR_TRANSIENT | View.NAVIGATION_BAR_TRANSIENT;
    private static final long AUTOHIDE_TIMEOUT_MS = 3000;

    /** The minimum delay in ms between reports of notification visibility. */
    private static final int VISIBILITY_REPORT_MIN_DELAY_MS = 500;

    /**
     * The delay to reset the hint text when the hint animation is finished running.
     */
    private static final int HINT_RESET_DELAY_MS = 1200;

    private static final AudioAttributes VIBRATION_ATTRIBUTES = new AudioAttributes.Builder()
            .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
            .setUsage(AudioAttributes.USAGE_ASSISTANCE_SONIFICATION)
            .build();

    public static final int FADE_KEYGUARD_START_DELAY = 100;
    public static final int FADE_KEYGUARD_DURATION = 300;

    /** Allow some time inbetween the long press for back and recents. */
    private static final int LOCK_TO_APP_GESTURE_TOLERENCE = 200;

    PhoneStatusBarPolicy mIconPolicy;

    // These are no longer handled by the policy, because we need custom strategies for them
    BluetoothControllerImpl mBluetoothController;
    SecurityControllerImpl mSecurityController;
    BatteryController mBatteryController;
    LocationControllerImpl mLocationController;
    NetworkControllerImpl mNetworkController;
    HotspotControllerImpl mHotspotController;
    RotationLockControllerImpl mRotationLockController;
    UserInfoController mUserInfoController;
    ZenModeController mZenModeController;
    CastControllerImpl mCastController;
    VolumeComponent mVolumeComponent;
    KeyguardUserSwitcher mKeyguardUserSwitcher;
    FlashlightController mFlashlightController;
    UserSwitcherController mUserSwitcherController;
    NextAlarmController mNextAlarmController;
    KeyguardMonitor mKeyguardMonitor;
    BrightnessMirrorController mBrightnessMirrorController;
    AccessibilityController mAccessibilityController;

    int mNaturalBarHeight = -1;
    int mIconSize = -1;
    int mIconHPadding = -1;
    Display mDisplay;
    Point mCurrentDisplaySize = new Point();

    StatusBarWindowView mStatusBarWindow;
    PhoneStatusBarView mStatusBarView;
    private int mStatusBarWindowState = WINDOW_STATE_SHOWING;
    private StatusBarWindowManager mStatusBarWindowManager;
    private UnlockMethodCache mUnlockMethodCache;
    private DozeServiceHost mDozeServiceHost;
    private boolean mScreenOnComingFromTouch;
    private PointF mScreenOnTouchLocation;

    int mPixelFormat;
    Object mQueueLock = new Object();

    // viewgroup containing the normal contents of the statusbar
    LinearLayout mStatusBarContents;

    // right-hand icons
    LinearLayout mSystemIconArea;
    LinearLayout mSystemIcons;

    // left-hand icons
    LinearLayout mStatusIcons;
    LinearLayout mStatusIconsKeyguard;

    // the icons themselves
    IconMerger mNotificationIcons;
    View mNotificationIconArea;

    // [+>
    View mMoreIcon;

    // expanded notifications
    NotificationPanelView mNotificationPanel; // the sliding/resizing panel within the notification window
    View mExpandedContents;
    int mNotificationPanelGravity;
    int mNotificationPanelMarginBottomPx;
    float mNotificationPanelMinHeightFrac;
    TextView mNotificationPanelDebugText;
    
    //screen shot
    ImageView mScreenshot;

	//huangjc win bar
	Dialog popupWindow;
	Dialog mLockpopupWindow;
	GridView mAppsGridView;
	private ListView lv_group;
	HorizontalScrollView mAppsScrollView;
	private ArrayList<HashMap<String, Object>> appslist=new ArrayList<HashMap<String, Object>>();
	private SimpleAdapter listadapter;
	private SimpleAdapter populistadapter;
	private WindowManager.LayoutParams wmDParams = null;
	public static boolean isMultiChange = false;
	
    // settings
    View mFlipSettingsView;
    private QSPanel mQSPanel;
	//app bar
	AppBarPanelView mAppBarPanel;

    // top bar
    StatusBarHeaderView mHeader;
    KeyguardStatusBarView mKeyguardStatusBar;
    View mKeyguardStatusView;
    KeyguardBottomAreaView mKeyguardBottomArea;
    boolean mLeaveOpenOnKeyguardHide;
    KeyguardIndicationController mKeyguardIndicationController;

    private boolean mKeyguardFadingAway;
    private long mKeyguardFadingAwayDelay;
    private long mKeyguardFadingAwayDuration;

    int mKeyguardMaxNotificationCount;

    // carrier/wifi label
    private TextView mCarrierLabel;
    private boolean mCarrierLabelVisible = false;
    private int mCarrierLabelHeight;
    private int mStatusBarHeaderHeight;

    private boolean mShowCarrierInPanel = false;

    private PhoneCards mPhoneCards;
    // SIM card switch notification and button
    LinearLayout mSimSwitchNotification;
    TextView mSimSwitchNotificationText;
    private int mSwitchType;
    private LinearLayout mSimSwitchContainer;
    private LinearLayout mSim1Container;
    private LinearLayout mSim2Container;
    private LinearLayout mAskContainer;
    private ImageView mSwitchSim1Button;
    private ImageView mSwitchSim2Button;
    private ImageView mSwitchAskButton;

    // position
    int[] mPositionTmp = new int[2];
    boolean mExpandedVisible;

    private int mNavigationBarWindowState = WINDOW_STATE_SHOWING;

    // the tracker view
    int mTrackingPosition; // the position of the top of the tracking view.

    // ticker
    private boolean mTickerEnabled;
    private Ticker mTicker;
    private View mTickerView;
    private boolean mTicking;

    // Tracking finger for opening/closing.
    int mEdgeBorder; // corresponds to R.dimen.status_bar_edge_ignore
    boolean mTracking;
    VelocityTracker mVelocityTracker;

    int[] mAbsPos = new int[2];
    ArrayList<Runnable> mPostCollapseRunnables = new ArrayList<>();

    // for disabling the status bar
    int mDisabled = 0;

    // tracking calls to View.setSystemUiVisibility()
    int mSystemUiVisibility = View.SYSTEM_UI_FLAG_VISIBLE;

    DisplayMetrics mDisplayMetrics = new DisplayMetrics();
    private WindowManager wm = null;
    WindowManager.LayoutParams mMultiModeParams;
	ImageView mMultiModeView;
	LinearLayout multiModeContainer;
	ImageView mHalfScreenController;
	LinearLayout mHSContainer;
	WindowManager.LayoutParams mHSCParams;
	private final boolean mEdgeRestriction = false;
	private  int MAX_RAN_MULCON = -1; 
	private  int MUL_CON_PAD = -1;
	private  int MUL_IMA_SIZE = -1;
	private  int MUL_CON_POS = -1;
	GestureDetector mMoveDector = null;
	GestureDetector mMultiModeDector = null;
	
	ImageView mFourScreenWController;
	LinearLayout mFSWSContainer;
	WindowManager.LayoutParams mFourScreenWCParams;
	GestureDetector mFourScreenWCMoveDector = null;
	
	ImageView mFourScreenHController;
	LinearLayout mFSHSContainer;
	WindowManager.LayoutParams mFourScreenHCParams;
	GestureDetector mFourScreenHCMoveDector = null;
	int mStatusBarHeight;
	
	private  int CENTER_BTN_WIDTH = 52;
	private  int CENTER_BTN_HEIGHT = 52;
	private  int CENTER_BTN_WIDTH_VALUE = 52;
	private  int CENTER_BTN_HEIGHT_VALUE = 52;
	private boolean isCenterBtnClick = false;
	WindowManager.LayoutParams mCenterBtnParams;
	ImageView mCenterBtnView;
	LinearLayout mCenterBtnContainer;
	GestureDetector mCenterBtnDector = null;
	
	WindowManager.LayoutParams mCircleMenuParams;
	private FourScreenCircleMenuView mFourScreenCircleMenuView;
	
	private FourScreenBackWindow mFourScreenBackWindow;
	
	private MinWindow mMinWindow;
	
	private final boolean IS_USE_BACK_WINDOW = false;
	private final boolean IS_USE_WHCONTROLS = false;
	
	private final boolean ONE_LEVEL_MENU = true;
	public static final HashMap<Integer, Integer> areaMap = new HashMap<Integer, Integer>();
	static {
		areaMap.put(0, 0);
		areaMap.put(1, 1);
		areaMap.put(2, 3);
		areaMap.put(3, 2);
	}
    // XXX: gesture research
    private final GestureRecorder mGestureRec = DEBUG_GESTURES
        ? new GestureRecorder("/sdcard/statusbar_gestures.dat")
        : null;

    private ScreenPinningRequest mScreenPinningRequest;

    private int mNavigationIconHints = 0;
    private HandlerThread mHandlerThread;

    // ensure quick settings is disabled until the current user makes it through the setup wizard
    private boolean mUserSetup = false;
    private ContentObserver mUserSetupObserver = new ContentObserver(new Handler()) {
        @Override
        public void onChange(boolean selfChange) {
            final boolean userSetup = 0 != Settings.Secure.getIntForUser(
                    mContext.getContentResolver(),
                    Settings.Secure.USER_SETUP_COMPLETE,
                    0 /*default */,
                    mCurrentUserId);
            if (MULTIUSER_DEBUG) Log.d(TAG, String.format("User setup changed: " +
                    "selfChange=%s userSetup=%s mUserSetup=%s",
                    selfChange, userSetup, mUserSetup));

            if (userSetup != mUserSetup) {
                mUserSetup = userSetup;
                if (!mUserSetup && mStatusBarView != null)
                    animateCollapseQuickSettings();
            }
        }
    };

    final private ContentObserver mHeadsUpObserver = new ContentObserver(mHandler) {
        @Override
        public void onChange(boolean selfChange) {
            boolean wasUsing = mUseHeadsUp;
            mUseHeadsUp = ENABLE_HEADS_UP && !mDisableNotificationAlerts
                    && Settings.Global.HEADS_UP_OFF != Settings.Global.getInt(
                    mContext.getContentResolver(), Settings.Global.HEADS_UP_NOTIFICATIONS_ENABLED,
                    Settings.Global.HEADS_UP_OFF);
            mHeadsUpTicker = mUseHeadsUp && 0 != Settings.Global.getInt(
                    mContext.getContentResolver(), SETTING_HEADS_UP_TICKER, 0);
            Log.d(TAG, "heads up is " + (mUseHeadsUp ? "enabled" : "disabled"));
            if (wasUsing != mUseHeadsUp) {
                if (!mUseHeadsUp) {
                    Log.d(TAG, "dismissing any existing heads up notification on disable event");
                    setHeadsUpVisibility(false);
                    mHeadsUpNotificationView.release();
                    removeHeadsUpView();
                } else {
                    addHeadsUpView();
                }
            }
        }
    };

    private int mInteractingWindows;
    private boolean mAutohideSuspended;
    private int mStatusBarMode;
    private int mNavigationBarMode;

    private ViewMediatorCallback mKeyguardViewMediatorCallback;
    private ScrimController mScrimController;
    private DozeScrimController mDozeScrimController;

    private final Runnable mAutohide = new Runnable() {
        @Override
        public void run() {
            int requested = mSystemUiVisibility & ~STATUS_OR_NAV_TRANSIENT;
            if (mSystemUiVisibility != requested) {
                notifyUiVisibilityChanged(requested);
            }
        }};

    private boolean mWaitingForKeyguardExit;
    private boolean mDozing;
    private boolean mScrimSrcModeEnabled;

    private Interpolator mLinearOutSlowIn;
    private Interpolator mLinearInterpolator = new LinearInterpolator();
    private Interpolator mBackdropInterpolator = new AccelerateDecelerateInterpolator();
    public static final Interpolator ALPHA_IN = new PathInterpolator(0.4f, 0f, 1f, 1f);
    public static final Interpolator ALPHA_OUT = new PathInterpolator(0f, 0f, 0.8f, 1f);

    private BackDropView mBackdrop;
    private ImageView mBackdropFront, mBackdropBack;
    private PorterDuffXfermode mSrcXferMode = new PorterDuffXfermode(PorterDuff.Mode.SRC);
    private PorterDuffXfermode mSrcOverXferMode = new PorterDuffXfermode(PorterDuff.Mode.SRC_OVER);

    private MediaSessionManager mMediaSessionManager;
    private MediaController mMediaController;
    private String mMediaNotificationKey;
    private MediaMetadata mMediaMetadata;
    private MediaController.Callback mMediaListener
            = new MediaController.Callback() {
        @Override
        public void onPlaybackStateChanged(PlaybackState state) {
            super.onPlaybackStateChanged(state);
            if (DEBUG_MEDIA) Log.v(TAG, "DEBUG_MEDIA: onPlaybackStateChanged: " + state);
        }

        @Override
        public void onMetadataChanged(MediaMetadata metadata) {
            super.onMetadataChanged(metadata);
            if (DEBUG_MEDIA) Log.v(TAG, "DEBUG_MEDIA: onMetadataChanged: " + metadata);
            mMediaMetadata = metadata;
            updateMediaMetaData(true);
        }
    };

    private final OnChildLocationsChangedListener mOnChildLocationsChangedListener =
            new OnChildLocationsChangedListener() {
        @Override
        public void onChildLocationsChanged(NotificationStackScrollLayout stackScrollLayout) {
            userActivity();
        }
    };

    private int mDisabledUnmodified;

    /** Keys of notifications currently visible to the user. */
    private final ArraySet<String> mCurrentlyVisibleNotifications = new ArraySet<String>();
    private long mLastVisibilityReportUptimeMs;

    private final ShadeUpdates mShadeUpdates = new ShadeUpdates();

    private int mDrawCount;
    private Runnable mLaunchTransitionEndRunnable;
    private boolean mLaunchTransitionFadingAway;
    private ExpandableNotificationRow mDraggedDownRow;

    // Fingerprint (as computed by getLoggingFingerprint() of the last logged state.
    private int mLastLoggedStateFingerprint;

    private static final int VISIBLE_LOCATIONS = ViewState.LOCATION_FIRST_CARD
            | ViewState.LOCATION_TOP_STACK_PEEKING
            | ViewState.LOCATION_MAIN_AREA
            | ViewState.LOCATION_BOTTOM_STACK_PEEKING;

    private final OnChildLocationsChangedListener mNotificationLocationsChangedListener =
            new OnChildLocationsChangedListener() {
                @Override
                public void onChildLocationsChanged(
                        NotificationStackScrollLayout stackScrollLayout) {
                    if (mHandler.hasCallbacks(mVisibilityReporter)) {
                        // Visibilities will be reported when the existing
                        // callback is executed.
                        return;
                    }
                    // Calculate when we're allowed to run the visibility
                    // reporter. Note that this timestamp might already have
                    // passed. That's OK, the callback will just be executed
                    // ASAP.
                    long nextReportUptimeMs =
                            mLastVisibilityReportUptimeMs + VISIBILITY_REPORT_MIN_DELAY_MS;
                    mHandler.postAtTime(mVisibilityReporter, nextReportUptimeMs);
                }
            };

    // Tracks notifications currently visible in mNotificationStackScroller and
    // emits visibility events via NoMan on changes.
    private final Runnable mVisibilityReporter = new Runnable() {
        private final ArrayList<String> mTmpNewlyVisibleNotifications = new ArrayList<String>();
        private final ArrayList<String> mTmpCurrentlyVisibleNotifications = new ArrayList<String>();

        @Override
        public void run() {
            mLastVisibilityReportUptimeMs = SystemClock.uptimeMillis();

            // 1. Loop over mNotificationData entries:
            //   A. Keep list of visible notifications.
            //   B. Keep list of previously hidden, now visible notifications.
            // 2. Compute no-longer visible notifications by removing currently
            //    visible notifications from the set of previously visible
            //    notifications.
            // 3. Report newly visible and no-longer visible notifications.
            // 4. Keep currently visible notifications for next report.
            ArrayList<Entry> activeNotifications = mNotificationData.getActiveNotifications();
            int N = activeNotifications.size();
            for (int i = 0; i < N; i++) {
                Entry entry = activeNotifications.get(i);
                String key = entry.notification.getKey();
                boolean previouslyVisible = mCurrentlyVisibleNotifications.contains(key);
                boolean currentlyVisible =
                        (mStackScroller.getChildLocation(entry.row) & VISIBLE_LOCATIONS) != 0;
                if (currentlyVisible) {
                    // Build new set of visible notifications.
                    mTmpCurrentlyVisibleNotifications.add(key);
                }
                if (!previouslyVisible && currentlyVisible) {
                    mTmpNewlyVisibleNotifications.add(key);
                }
            }
            ArraySet<String> noLongerVisibleNotifications = mCurrentlyVisibleNotifications;
            noLongerVisibleNotifications.removeAll(mTmpCurrentlyVisibleNotifications);

            logNotificationVisibilityChanges(
                    mTmpNewlyVisibleNotifications, noLongerVisibleNotifications);

            mCurrentlyVisibleNotifications.clear();
            mCurrentlyVisibleNotifications.addAll(mTmpCurrentlyVisibleNotifications);

            mTmpNewlyVisibleNotifications.clear();
            mTmpCurrentlyVisibleNotifications.clear();
        }
    };

    private final View.OnClickListener mOverflowClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            goToLockedShade(null);
        }
    };

    @Override
    public void start() {
        mDisplay = ((WindowManager)mContext.getSystemService(Context.WINDOW_SERVICE))
                .getDefaultDisplay();
        updateDisplaySize();
        mScrimSrcModeEnabled = mContext.getResources().getBoolean(
                R.bool.config_status_bar_scrim_behind_use_src);
        super.start(); // calls createAndAddWindows()

        mMediaSessionManager
                = (MediaSessionManager) mContext.getSystemService(Context.MEDIA_SESSION_SERVICE);
        // TODO: use MediaSessionManager.SessionListener to hook us up to future updates
        // in session state

        addNavigationBar();
        changeBarHideStatus();

        // Lastly, call to the icon policy to install/update all the icons.
        mIconPolicy = new PhoneStatusBarPolicy(mContext, mCastController, mHotspotController);
        mSettingsObserver.onChange(false); // set up
        
	mContext.getContentResolver().registerContentObserver(Settings.System.getUriFor(Dual_Screen_KEY), false,mDualScreenValueObserver);
	mContext.getContentResolver().registerContentObserver(Settings.System.getUriFor(Dual_Screen_Icon_used_KEY), false,mDualScreenIconUsedObserver);

	
        mHeadsUpObserver.onChange(true); // set up
        if (ENABLE_HEADS_UP) {
            mContext.getContentResolver().registerContentObserver(
                    Settings.Global.getUriFor(Settings.Global.HEADS_UP_NOTIFICATIONS_ENABLED), true,
                    mHeadsUpObserver);
            mContext.getContentResolver().registerContentObserver(
                    Settings.Global.getUriFor(SETTING_HEADS_UP_TICKER), true,
                    mHeadsUpObserver);
        }
			
			mContext.getContentResolver().registerContentObserver(
                Settings.System.getUriFor(Settings.System.MULTI_WINDOW_CONFIG), false,
                mMultiConfigObserver);
			mContext.getContentResolver().registerContentObserver(
                Settings.System.getUriFor(Settings.System.EXTER_KEYBOARD_CONFIG), false,
                mExtkeyboardObserver);
        mUnlockMethodCache = UnlockMethodCache.getInstance(mContext);
        mUnlockMethodCache.addListener(this);
        startKeyguard();

        mDozeServiceHost = new DozeServiceHost();
        putComponent(DozeHost.class, mDozeServiceHost);
        putComponent(PhoneStatusBar.class, this);

        setControllerUsers();

        notifyUserAboutHiddenNotifications();

        mScreenPinningRequest = new ScreenPinningRequest(mContext);
        mPhoneCards = PhoneCards.getInstance(mContext);
    }
final Object mScreenshotLock = new Object();
    ServiceConnection mScreenshotConnection = null;

    final Runnable mScreenshotTimeout = new Runnable() {
		        @Override public void run() {
				            synchronized (mScreenshotLock) {
						                if (mScreenshotConnection != null) {
								                    mContext.unbindService(mScreenshotConnection);
								                    mScreenshotConnection = null;
								                }
								            }
								        }
								    };
    private void takeScreenshot() {
		        String imageDir=Settings.System.getString(mContext.getContentResolver(), Settings.System.SCREENSHOT_LOCATION);
				File file=new File(imageDir+UserHandle.myUserId()+"/Screenshots");
		        String text=null;
				Log.e(">>>>>>","imageDir="+imageDir);
		
		        file.mkdir();
		
		        if(!file.exists()){
				           if(imageDir.equals("/mnt/sdcard")){
						                text=mContext.getResources().getString(R.string.sdcard_unmount);
						           }else if(imageDir.equals("/mnt/external_sd")){
								                text=mContext.getResources().getString(R.string.external_sd_unmount);
								           }else if(imageDir.equals("/mnt/usb_storage")){
										                text=mContext.getResources().getString(R.string.usb_storage_unmount);
										           }
										           Toast.makeText(mContext, text, 3000).show();
										           return;
										        }     
        synchronized (mScreenshotLock) {
		            if (mScreenshotConnection != null) {
				                return;
				            } 
				            ComponentName cn = new ComponentName("com.android.systemui",
				                    "com.android.systemui.screenshot.TakeScreenshotService");
				            Intent intent = new Intent();
				            intent.setComponent(cn);
				            ServiceConnection conn = new ServiceConnection() {
						                @Override
						                public void onServiceConnected(ComponentName name, IBinder service) {
								                    synchronized (mScreenshotLock) {
										                       if (mScreenshotConnection != this) {
			                                             return;
												                        }																					
                        Messenger messenger = new Messenger(service);
                        Message msg = Message.obtain(null, 1);
                        final ServiceConnection myConn = this;
                        Handler h = new Handler(mHandler.getLooper()) {
		                            @Override
		                            public void handleMessage(Message msg) {
				                                synchronized (mScreenshotLock) {
				                                    if (mScreenshotConnection == myConn) {
						                                        mContext.unbindService(mScreenshotConnection);
						                                        mScreenshotConnection = null;
						                                        mHandler.removeCallbacks(mScreenshotTimeout);
						                                    }
						                                }
						                            }
								                        };
                        msg.replyTo = new Messenger(h);
                        msg.arg1=0;
                        msg.arg2=1;
                       // if (mStatusBar != null && mStatusBar.isVisibleLw())
                       //     msg.arg1 = 1;
                       // if (mNavigationBar != null && mNavigationBar.isVisibleLw())
                       //     msg.arg2 = 1;
                        try {
		                            messenger.send(msg);
		                        } catch (RemoteException e) {
				                        }
				                    }
				                }
                @Override
                public void onServiceDisconnected(ComponentName name) {}
            };
            if (mContext.bindService(intent, conn, Context.BIND_AUTO_CREATE)) {
		                mScreenshotConnection = conn;
		                mHandler.postDelayed(mScreenshotTimeout, 10000);
		            }
		        }
		    }
	private boolean isWinShow = true;
	private BroadcastReceiver winreceiver=new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
	               // TODO Auto-generated method stub
	 if(mContext.getResources().getConfiguration().enableMultiWindow()){
		        String action=intent.getAction();
			String cmp = intent.getExtras().getString("cmp");
			LOGD("wintask action:"+action+"==cmp:"+cmp);
			if(action.equals("rk.android.wintask.SHOW")){
                        LOGD("wintask receive app start...");
		try {
			 Thread.sleep(600);
		} catch (Exception ex) {
		}
	          isWinShow = true;
	          UpdateAppsList(cmp);
   }
	if(action.equals("rk.android.wintask.FINISH")){
            LOGD("wintask receive app finsh...");
		try {
		    Thread.sleep(500);
		} catch (Exception ex) {
			//Log.e(TAG,"Exception:" +ex.getMessage());
		}
		isWinShow = false;
		UpdateAppsList(cmp);
	}
			}
	 }
						                
	};	
	private BroadcastReceiver packagereceiver=new BroadcastReceiver() {
		    @Override
		    public void onReceive(Context context, Intent intent) {
			// TODO Auto-generated method stub
			if(mContext.getResources().getConfiguration().enableMultiWindow()){
		        String action=intent.getAction();
			String cmp = intent.getData().getSchemeSpecificPart();
		        LOGD("wintask action:"+action+"==cmp:"+cmp);
			if(action.equals(Intent.ACTION_PACKAGE_REMOVED)){
	            LOGD("wintask receive PACKAGE_REMOVED...");
				 try {
						 Thread.sleep(500);
				 } catch (Exception ex) {
						//Log.e(TAG,"Exception:" +ex.getMessage());
				}
				    isWinShow = false;
					UpdateAppsList(cmp);
													 
                                              
				}
		        }	     
		 }           
	}; 
    private BroadcastReceiver receiver=new BroadcastReceiver() {
		
		                @Override
		                public void onReceive(Context context, Intent intent) {
	                        // TODO Auto-generated method stub
                    String action=intent.getAction();
                    Log.d("screenshot",action);
                    if(action.equals("rk.android.screenshot.SHOW")){
                        boolean show=intent.getBooleanExtra("show", false);
                         if(show){
		                       Log.d("screenshot","show screenshot button");
		                   mNavigationBarView.getScreenshotButton().setVisibility(View.VISIBLE);
		                         }else{
			                    Log.d("screenshot","disable screenshot button");
				           mNavigationBarView.getScreenshotButton().setVisibility(View.INVISIBLE);
		                         }
		                        }else{
				                            takeScreenshot();
				                        }
				                }
						        };	
    // ================================================================================
    // Constructing the view
    // ================================================================================
    protected PhoneStatusBarView makeStatusBarView() {
        final Context context = mContext;
        IntentFilter intentfilter=new IntentFilter();
        intentfilter.addAction("rk.android.screenshot.SHOW");
        intentfilter.addAction("rk.android.screenshot.ACTION");
        context.registerReceiver(receiver, intentfilter);	

		IntentFilter winintentfilter=new IntentFilter();
		winintentfilter.addAction("rk.android.wintask.SHOW");
		winintentfilter.addAction("rk.android.wintask.FINISH");
        context.registerReceiver(winreceiver, winintentfilter);	
    
    IntentFilter packagefilter=new IntentFilter();
		packagefilter.addAction(Intent.ACTION_PACKAGE_ADDED);
    packagefilter.addAction(Intent.ACTION_PACKAGE_REMOVED);
    packagefilter.addDataScheme("package");
        context.registerReceiver(packagereceiver, packagefilter);	

        Resources res = context.getResources();

        updateDisplaySize(); // populates mDisplayMetrics
        updateResources();

        mIconSize = res.getDimensionPixelSize(com.android.internal.R.dimen.status_bar_icon_size);

        mStatusBarWindow = (StatusBarWindowView) View.inflate(context,
                R.layout.super_status_bar, null);
        mStatusBarWindow.mService = this;
        mStatusBarWindow.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                checkUserAutohide(v, event);
                if (event.getAction() == MotionEvent.ACTION_DOWN) {
                    if (mExpandedVisible) {
                        animateCollapsePanels();
                    }
                }
                return mStatusBarWindow.onTouchEvent(event);
            }});

        mStatusBarView = (PhoneStatusBarView) mStatusBarWindow.findViewById(R.id.status_bar);
        mStatusBarView.setBar(this);

        PanelHolder holder = (PanelHolder) mStatusBarWindow.findViewById(R.id.panel_holder);
        mStatusBarView.setPanelHolder(holder);

		mAppBarPanel = (AppBarPanelView)mStatusBarWindow.findViewById(R.id.appbar_panel);
		mAppBarPanel.setStatusBar(this);

        mNotificationPanel = (NotificationPanelView) mStatusBarWindow.findViewById(R.id.notification_panel);
        mNotificationPanel.setStatusBar(this);

        if (!ActivityManager.isHighEndGfx()) {
            mStatusBarWindow.setBackground(null);
            mNotificationPanel.setBackground(new FastColorDrawable(context.getResources().getColor(
                    R.color.notification_panel_solid_background)));
			mAppBarPanel.setBackground(new FastColorDrawable(context.getResources().getColor(
                    R.color.notification_panel_solid_background)));
        }
        if (ENABLE_HEADS_UP) {
            mHeadsUpNotificationView =
                    (HeadsUpNotificationView) View.inflate(context, R.layout.heads_up, null);
            mHeadsUpNotificationView.setVisibility(View.GONE);
            mHeadsUpNotificationView.setBar(this);
        }
        if (MULTIUSER_DEBUG) {
            mNotificationPanelDebugText = (TextView) mNotificationPanel.findViewById(
                    R.id.header_debug_info);
            mNotificationPanelDebugText.setVisibility(View.VISIBLE);
        }

        updateShowSearchHoldoff();

        try {
            boolean showNav = mWindowManagerService.hasNavigationBar();
            if (DEBUG) Log.v(TAG, "hasNavigationBar=" + showNav);
            if (showNav) {
            	//haungjc:win bar
                if(mContext.getResources().getConfiguration().enableMultiWindow()){
            		mNavigationBarView =
                    (NavigationBarView) View.inflate(mContext, R.layout.navigation_bar_win, null);
              }else{
                mNavigationBarView =
                    (NavigationBarView) View.inflate(mContext, R.layout.navigation_bar, null);
              }

                mNavigationBarView.setDisabledFlags(mDisabled);
                mNavigationBarView.setBar(this);
                mNavigationBarView.setOnVerticalChangedListener(
                        new NavigationBarView.OnVerticalChangedListener() {
                    @Override
                    public void onVerticalChanged(boolean isVertical) {
                        if (mSearchPanelView != null) {
                            mSearchPanelView.setHorizontal(isVertical);
                        }
                        mNotificationPanel.setQsScrimEnabled(!isVertical);
                    }
                });
                mNavigationBarView.setOnTouchListener(new View.OnTouchListener() {
                    @Override
                    public boolean onTouch(View v, MotionEvent event) {
                        checkUserAutohide(v, event);
                        return false;
                    }});			   
            }
        } catch (RemoteException ex) {
            // no window manager? good luck with that
        }

        // figure out which pixel-format to use for the status bar.
        mPixelFormat = PixelFormat.OPAQUE;

        mSystemIconArea = (LinearLayout) mStatusBarView.findViewById(R.id.system_icon_area);
        mSystemIcons = (LinearLayout) mStatusBarView.findViewById(R.id.system_icons);
        mStatusIcons = (LinearLayout)mStatusBarView.findViewById(R.id.statusIcons);
        mNotificationIconArea = mStatusBarView.findViewById(R.id.notification_icon_area_inner);
        mNotificationIcons = (IconMerger)mStatusBarView.findViewById(R.id.notificationIcons);
        mMoreIcon = mStatusBarView.findViewById(R.id.moreIcon);
        mNotificationIcons.setOverflowIndicator(mMoreIcon);
        mStatusBarContents = (LinearLayout)mStatusBarView.findViewById(R.id.status_bar_contents);

        mSimSwitchNotification = (LinearLayout) mStatusBarView.findViewById(
                R.id.simSwitchNotification);
        mSimSwitchNotificationText = (TextView) mStatusBarView.findViewById(
                R.id.simSwitchNotificationText);
        mSimSwitchContainer = (LinearLayout) mStatusBarWindow.findViewById(
                R.id.sim_switch_container);
        mSim1Container = (LinearLayout)mStatusBarWindow.findViewById(R.id.sim1_container);
        mSim2Container = (LinearLayout)mStatusBarWindow.findViewById(R.id.sim2_container);
        mAskContainer = (LinearLayout)mStatusBarWindow.findViewById(R.id.ask_container);
        mSwitchSim1Button = (ImageView)mStatusBarWindow.findViewById(R.id.sim1_switch_button);
        mSwitchSim2Button = (ImageView)mStatusBarWindow.findViewById(R.id.sim2_switch_button);
        mSwitchAskButton = (ImageView)mStatusBarWindow.findViewById(R.id.ask_switch_button);
        mSwitchSim1Button.setOnClickListener(mSimSwitchListener);
        mSwitchSim2Button.setOnClickListener(mSimSwitchListener);
        mSwitchAskButton.setOnClickListener(mSimSwitchListener);

        mStackScroller = (NotificationStackScrollLayout) mStatusBarWindow.findViewById(
                R.id.notification_stack_scroller);
        mStackScroller.setLongPressListener(getNotificationLongClicker());
        mStackScroller.setPhoneStatusBar(this);

        mKeyguardIconOverflowContainer =
                (NotificationOverflowContainer) LayoutInflater.from(mContext).inflate(
                        R.layout.status_bar_notification_keyguard_overflow, mStackScroller, false);
        mKeyguardIconOverflowContainer.setOnActivatedListener(this);
        mKeyguardIconOverflowContainer.setOnClickListener(mOverflowClickListener);
        mStackScroller.addView(mKeyguardIconOverflowContainer);

        SpeedBumpView speedBump = (SpeedBumpView) LayoutInflater.from(mContext).inflate(
                        R.layout.status_bar_notification_speed_bump, mStackScroller, false);
        mStackScroller.setSpeedBumpView(speedBump);
        mEmptyShadeView = (EmptyShadeView) LayoutInflater.from(mContext).inflate(
                R.layout.status_bar_no_notifications, mStackScroller, false);
        mStackScroller.setEmptyShadeView(mEmptyShadeView);
        mDismissView = (DismissView) LayoutInflater.from(mContext).inflate(
                R.layout.status_bar_notification_dismiss_all, mStackScroller, false);
        mDismissView.setOnButtonClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                clearAllNotifications();
            }
        });
        mStackScroller.setDismissView(mDismissView);
        mExpandedContents = mStackScroller;

        mBackdrop = (BackDropView) mStatusBarWindow.findViewById(R.id.backdrop);
        mBackdropFront = (ImageView) mBackdrop.findViewById(R.id.backdrop_front);
        mBackdropBack = (ImageView) mBackdrop.findViewById(R.id.backdrop_back);

        ScrimView scrimBehind = (ScrimView) mStatusBarWindow.findViewById(R.id.scrim_behind);
        ScrimView scrimInFront = (ScrimView) mStatusBarWindow.findViewById(R.id.scrim_in_front);
        mScrimController = new ScrimController(scrimBehind, scrimInFront, mScrimSrcModeEnabled);
        mScrimController.setBackDropView(mBackdrop);
        mStatusBarView.setScrimController(mScrimController);
        mDozeScrimController = new DozeScrimController(mScrimController, context);

        mHeader = (StatusBarHeaderView) mStatusBarWindow.findViewById(R.id.header);
        mHeader.setActivityStarter(this);
        mKeyguardStatusBar = (KeyguardStatusBarView) mStatusBarWindow.findViewById(R.id.keyguard_header);
        mStatusIconsKeyguard = (LinearLayout) mKeyguardStatusBar.findViewById(R.id.statusIcons);
        mKeyguardStatusView = mStatusBarWindow.findViewById(R.id.keyguard_status_view);
        mKeyguardBottomArea =
                (KeyguardBottomAreaView) mStatusBarWindow.findViewById(R.id.keyguard_bottom_area);
        mKeyguardBottomArea.setActivityStarter(this);
        mKeyguardIndicationController = new KeyguardIndicationController(mContext,
                (KeyguardIndicationTextView) mStatusBarWindow.findViewById(
                        R.id.keyguard_indication_text));
        mKeyguardBottomArea.setKeyguardIndicationController(mKeyguardIndicationController);

        mTickerEnabled = res.getBoolean(R.bool.enable_ticker);
        if (mTickerEnabled) {
            final ViewStub tickerStub = (ViewStub) mStatusBarView.findViewById(R.id.ticker_stub);
            if (tickerStub != null) {
                mTickerView = tickerStub.inflate();
                mTicker = new MyTicker(context, mStatusBarView);

                TickerView tickerView = (TickerView) mStatusBarView.findViewById(R.id.tickerText);
                tickerView.mTicker = mTicker;
            }
        }

        mEdgeBorder = res.getDimensionPixelSize(R.dimen.status_bar_edge_ignore);

        // set the inital view visibility
        setAreThereNotifications();

        // Background thread for any controllers that need it.
        mHandlerThread = new HandlerThread(TAG, Process.THREAD_PRIORITY_BACKGROUND);
        mHandlerThread.start();

        // Other icons
        mLocationController = new LocationControllerImpl(mContext); // will post a notification
        mBatteryController = new BatteryController(mContext);
		mBatteryController.setPercentageView((TextView) mStatusBarWindow.findViewById(R.id.battery_percentage));
        mBatteryController.addStateChangedCallback(new BatteryStateChangeCallback() {
            @Override
            public void onPowerSaveChanged() {
                mHandler.post(mCheckBarModes);
                if (mDozeServiceHost != null) {
                    mDozeServiceHost.firePowerSaveChanged(mBatteryController.isPowerSave());
                }
            }
            @Override
            public void onBatteryLevelChanged(int level, boolean pluggedIn, boolean charging) {
                // noop
            }
        });
        mNetworkController = new NetworkControllerImpl(mContext);
        mHotspotController = new HotspotControllerImpl(mContext);
        mBluetoothController = new BluetoothControllerImpl(mContext, mHandlerThread.getLooper());
        mSecurityController = new SecurityControllerImpl(mContext);
        if (mContext.getResources().getBoolean(R.bool.config_showRotationLock)) {
            mRotationLockController = new RotationLockControllerImpl(mContext);
        }
        mUserInfoController = new UserInfoController(mContext);
        mVolumeComponent = getComponent(VolumeComponent.class);
        if (mVolumeComponent != null) {
            mZenModeController = mVolumeComponent.getZenController();
        }
        mCastController = new CastControllerImpl(mContext);
        final SignalClusterView signalCluster =
                (SignalClusterView) mStatusBarView.findViewById(R.id.signal_cluster);
        final SignalClusterView signalClusterKeyguard =
                (SignalClusterView) mKeyguardStatusBar.findViewById(R.id.signal_cluster);
        final SignalClusterView signalClusterQs =
                (SignalClusterView) mHeader.findViewById(R.id.signal_cluster);
        mNetworkController.addSignalCluster(signalCluster);
        mNetworkController.addSignalCluster(signalClusterKeyguard);
        mNetworkController.addSignalCluster(signalClusterQs);
        signalCluster.setSecurityController(mSecurityController);
        signalCluster.setNetworkController(mNetworkController);
        signalClusterKeyguard.setSecurityController(mSecurityController);
        signalClusterKeyguard.setNetworkController(mNetworkController);
        signalClusterQs.setSecurityController(mSecurityController);
        signalClusterQs.setNetworkController(mNetworkController);
        final boolean isAPhone = mNetworkController.hasVoiceCallingFeature();
        if (isAPhone) {
            mNetworkController.addEmergencyListener(new NetworkControllerImpl.EmergencyListener() {
                @Override
                public void setEmergencyCallsOnly(boolean emergencyOnly) {
                    mHeader.setShowEmergencyCallsOnly(emergencyOnly);
                }
            });
        }

        mCarrierLabel = (TextView)mStatusBarWindow.findViewById(R.id.carrier_label);
        mShowCarrierInPanel = (mCarrierLabel != null);
        if (DEBUG) Log.v(TAG, "carrierlabel=" + mCarrierLabel + " show=" + mShowCarrierInPanel);
        if (mShowCarrierInPanel) {
            mCarrierLabel.setVisibility(mCarrierLabelVisible ? View.VISIBLE : View.INVISIBLE);

            mNetworkController.addCarrierLabel(new NetworkControllerImpl.CarrierLabelListener() {
                @Override
                public void setCarrierLabel(String label) {
                    mCarrierLabel.setText(label);
                    if (mNetworkController.hasMobileDataFeature()) {
                        if (TextUtils.isEmpty(label)) {
                            mCarrierLabel.setVisibility(View.GONE);
                        } else {
                            mCarrierLabel.setVisibility(View.VISIBLE);
                        }
                    }
                }
            });
        }

        mFlashlightController = new FlashlightController(mContext);
        mKeyguardBottomArea.setFlashlightController(mFlashlightController);
        mKeyguardBottomArea.setPhoneStatusBar(this);
        mAccessibilityController = new AccessibilityController(mContext);
        mKeyguardBottomArea.setAccessibilityController(mAccessibilityController);
        mNextAlarmController = new NextAlarmController(mContext);
        mKeyguardMonitor = new KeyguardMonitor();
        if (UserSwitcherController.isUserSwitcherAvailable(UserManager.get(mContext))) {
            mUserSwitcherController = new UserSwitcherController(mContext, mKeyguardMonitor);
        }
        mKeyguardUserSwitcher = new KeyguardUserSwitcher(mContext,
                (ViewStub) mStatusBarWindow.findViewById(R.id.keyguard_user_switcher),
                mKeyguardStatusBar, mNotificationPanel, mUserSwitcherController);


        // Set up the quick settings tile panel
        mQSPanel = (QSPanel) mStatusBarWindow.findViewById(R.id.quick_settings_panel);
        if (mQSPanel != null) {
            final QSTileHost qsh = new QSTileHost(mContext, this,
                    mBluetoothController, mLocationController, mRotationLockController,
                    mNetworkController, mZenModeController, mHotspotController,
                    mCastController, mFlashlightController,
                    mUserSwitcherController, mKeyguardMonitor,
                    mSecurityController);
            mQSPanel.setHost(qsh);
            mQSPanel.setTiles(qsh.getTiles());
            mBrightnessMirrorController = new BrightnessMirrorController(mStatusBarWindow);
            mQSPanel.setBrightnessMirror(mBrightnessMirrorController);
            mHeader.setQSPanel(mQSPanel);
            qsh.setCallback(new QSTileHost.Callback() {
                @Override
                public void onTilesChanged() {
                    mQSPanel.setTiles(qsh.getTiles());
                }
            });
        }

        // User info. Trigger first load.
        mHeader.setUserInfoController(mUserInfoController);
        mKeyguardStatusBar.setUserInfoController(mUserInfoController);
        mUserInfoController.reloadUserInfo();

        mHeader.setBatteryController(mBatteryController);
        ((BatteryMeterView) mStatusBarView.findViewById(R.id.battery)).setBatteryController(
                mBatteryController);
        mKeyguardStatusBar.setBatteryController(mBatteryController);
        mHeader.setNextAlarmController(mNextAlarmController);

        PowerManager pm = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
        mBroadcastReceiver.onReceive(mContext,
                new Intent(pm.isScreenOn() ? Intent.ACTION_SCREEN_ON : Intent.ACTION_SCREEN_OFF));

        // receive broadcasts
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        filter.addAction(Intent.ACTION_SCREEN_ON);
        if (DEBUG_MEDIA_FAKE_ARTWORK) {
            filter.addAction("fake_artwork");
        }
        filter.addAction(ACTION_DEMO);
        filter.addAction("com.tchip.changeBarHideStatus");
        context.registerReceiverAsUser(mBroadcastReceiver, UserHandle.ALL, filter, null, null);

        // listen for USER_SETUP_COMPLETE setting (per-user)
        resetUserSetupObserver();

        startGlyphRasterizeHack();
        return mStatusBarView;
    }

    private void clearAllNotifications() {

        // animate-swipe all dismissable notifications, then animate the shade closed
        int numChildren = mStackScroller.getChildCount();

        final ArrayList<View> viewsToHide = new ArrayList<View>(numChildren);
        for (int i = 0; i < numChildren; i++) {
            final View child = mStackScroller.getChildAt(i);
            if (mStackScroller.canChildBeDismissed(child)) {
                if (child.getVisibility() == View.VISIBLE) {
                    viewsToHide.add(child);
                }
            }
        }
        if (viewsToHide.isEmpty()) {
            animateCollapsePanels(CommandQueue.FLAG_EXCLUDE_NONE);
            return;
        }

        addPostCollapseAction(new Runnable() {
            @Override
            public void run() {
                try {
                    mBarService.onClearAllNotifications(mCurrentUserId);
                } catch (Exception ex) { }
            }
        });

        performDismissAllAnimations(viewsToHide);

    }

    private void performDismissAllAnimations(ArrayList<View> hideAnimatedList) {
        Runnable animationFinishAction = new Runnable() {
            @Override
            public void run() {
                mStackScroller.post(new Runnable() {
                    @Override
                    public void run() {
                        mStackScroller.setDismissAllInProgress(false);
                    }
                });
                animateCollapsePanels(CommandQueue.FLAG_EXCLUDE_NONE);
            }
        };

        // let's disable our normal animations
        mStackScroller.setDismissAllInProgress(true);

        // Decrease the delay for every row we animate to give the sense of
        // accelerating the swipes
        int rowDelayDecrement = 10;
        int currentDelay = 140;
        int totalDelay = 180;
        int numItems = hideAnimatedList.size();
        for (int i = numItems - 1; i >= 0; i--) {
            View view = hideAnimatedList.get(i);
            Runnable endRunnable = null;
            if (i == 0) {
                endRunnable = animationFinishAction;
            }
            mStackScroller.dismissViewAnimated(view, endRunnable, totalDelay, 260);
            currentDelay = Math.max(50, currentDelay - rowDelayDecrement);
            totalDelay += currentDelay;
        }
    }

    /**
     * Hack to improve glyph rasterization for scaled text views.
     */
    private void startGlyphRasterizeHack() {
        mStatusBarView.getViewTreeObserver().addOnPreDrawListener(
                new ViewTreeObserver.OnPreDrawListener() {
            @Override
            public boolean onPreDraw() {
                if (mDrawCount == 1) {
                    mStatusBarView.getViewTreeObserver().removeOnPreDrawListener(this);
                    HardwareCanvas.setProperty("extraRasterBucket",
                            Float.toString(StackScrollAlgorithm.DIMMED_SCALE));
                    HardwareCanvas.setProperty("extraRasterBucket", Float.toString(
                            mContext.getResources().getDimensionPixelSize(
                                    R.dimen.qs_time_collapsed_size)
                            / mContext.getResources().getDimensionPixelSize(
                                    R.dimen.qs_time_expanded_size)));
                }
                mDrawCount++;
                return true;
            }
        });
    }

    @Override
    protected void setZenMode(int mode) {
        super.setZenMode(mode);
        if (mIconPolicy != null) {
            mIconPolicy.setZenMode(mode);
        }
    }

    private void startKeyguard() {
        KeyguardViewMediator keyguardViewMediator = getComponent(KeyguardViewMediator.class);
        mStatusBarKeyguardViewManager = keyguardViewMediator.registerStatusBar(this,
                mStatusBarWindow, mStatusBarWindowManager, mScrimController);
        mKeyguardViewMediatorCallback = keyguardViewMediator.getViewMediatorCallback();
    }

    @Override
    protected View getStatusBarView() {
        return mStatusBarView;
    }

    public StatusBarWindowView getStatusBarWindow() {
        return mStatusBarWindow;
    }

    @Override
    protected WindowManager.LayoutParams getSearchLayoutParams(LayoutParams layoutParams) {
        boolean opaque = false;
        WindowManager.LayoutParams lp = new WindowManager.LayoutParams(
                LayoutParams.MATCH_PARENT,
                LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.TYPE_NAVIGATION_BAR_PANEL,
                WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                | WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM
                | WindowManager.LayoutParams.FLAG_SPLIT_TOUCH,
                (opaque ? PixelFormat.OPAQUE : PixelFormat.TRANSLUCENT));
        if (ActivityManager.isHighEndGfx()) {
            lp.flags |= WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
        }
        lp.gravity = Gravity.BOTTOM | Gravity.START;
        lp.setTitle("SearchPanel");
        lp.softInputMode = WindowManager.LayoutParams.SOFT_INPUT_STATE_UNCHANGED
        | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING;
        return lp;
    }

    @Override
    protected void updateSearchPanel() {
        super.updateSearchPanel();
        if (mNavigationBarView != null) {
            mNavigationBarView.setDelegateView(mSearchPanelView);
        }
    }

    @Override
    public void showSearchPanel() {
        super.showSearchPanel();
        mHandler.removeCallbacks(mShowSearchPanel);

        // we want to freeze the sysui state wherever it is
        mSearchPanelView.setSystemUiVisibility(mSystemUiVisibility);

        if (mNavigationBarView != null) {
            WindowManager.LayoutParams lp =
                (android.view.WindowManager.LayoutParams) mNavigationBarView.getLayoutParams();
            lp.flags &= ~WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL;
            mWindowManager.updateViewLayout(mNavigationBarView, lp);
        }
    }

    @Override
    public void hideSearchPanel() {
        super.hideSearchPanel();
        if (mNavigationBarView != null) {
            WindowManager.LayoutParams lp =
                (android.view.WindowManager.LayoutParams) mNavigationBarView.getLayoutParams();
            lp.flags |= WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL;
            mWindowManager.updateViewLayout(mNavigationBarView, lp);
        }
    }

    public int getStatusBarHeight() {
        if (mNaturalBarHeight < 0) {
            final Resources res = mContext.getResources();
            mNaturalBarHeight =
                    res.getDimensionPixelSize(com.android.internal.R.dimen.status_bar_height);
        }
        return mNaturalBarHeight;
    }

    private View.OnClickListener mRecentsClickListener = new View.OnClickListener() {
        public void onClick(View v) {
            awakenDreams();
            toggleRecentApps();
        }
    };

    private long mLastLockToAppLongPress;
    private View.OnLongClickListener mLongPressBackRecentsListener =
            new View.OnLongClickListener() {
        @Override
        public boolean onLongClick(View v) {
            handleLongPressBackRecents(v);
            return true;
        }
    };

    private int mShowSearchHoldoff = 0;
    private Runnable mShowSearchPanel = new Runnable() {
        public void run() {
            showSearchPanel();
            awakenDreams();
        }
    };

    View.OnTouchListener mHomeActionListener = new View.OnTouchListener() {
        public boolean onTouch(View v, MotionEvent event) {
            switch(event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                if (!shouldDisableNavbarGestures()) {
                    mHandler.removeCallbacks(mShowSearchPanel);
                    mHandler.postDelayed(mShowSearchPanel, mShowSearchHoldoff);
                }
            break;

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                mHandler.removeCallbacks(mShowSearchPanel);
                awakenDreams();
            break;
        }
        return false;
        }
    };

    private void awakenDreams() {
        if (mDreamManager != null) {
            try {
                mDreamManager.awaken();
            } catch (RemoteException e) {
                // fine, stay asleep then
            }
        }
    }

    private void prepareNavigationBarView() {
        mNavigationBarView.reorient();

        mNavigationBarView.getRecentsButton().setOnClickListener(mRecentsClickListener);
        boolean show=Settings.System.getInt(mContext.getContentResolver(),
                        Settings.System.SCREENSHOT_BUTTON_SHOW, 0)==1;
        if(show){
		                     mNavigationBarView.getScreenshotButton().setVisibility(View.VISIBLE);
		                }else{
				                     mNavigationBarView.getScreenshotButton().setVisibility(View.INVISIBLE);
				                }								
	    mNavigationBarView.getScreenshotButton().setOnTouchListener(mScreenshotPreloadOnTouchListener);     
        mNavigationBarView.getRecentsButton().setOnTouchListener(mRecentsPreloadOnTouchListener);
        mNavigationBarView.getRecentsButton().setLongClickable(true);
        mNavigationBarView.getRecentsButton().setOnLongClickListener(mLongPressBackRecentsListener);
        mNavigationBarView.getPowerButton().setOnTouchListener(mPowerPreloadOnTouchListener);
        mNavigationBarView.getBackButton().setLongClickable(true);
        mNavigationBarView.getBackButton().setOnLongClickListener(mLongPressBackRecentsListener);
        mNavigationBarView.getHomeButton().setOnTouchListener(mHomeActionListener);
		mNavigationBarView.getHomeButton().setLongClickable(true);
        mNavigationBarView.getHomeButton().setOnLongClickListener(mLongPressBackRecentsListener);
		
		if(mNavigationBarView != null && mNavigationBarView.getDisplaycopyButton() != null){
			
			final boolean enable = (getDualScreenValue() != 0) && (getDualScreenIconUsedValue() != 0);
			if(enable){
				mNavigationBarView.getDisplaycopyButton().setVisibility(View.VISIBLE);
			}else{
				mNavigationBarView.getDisplaycopyButton().setVisibility(View.GONE);
			}
			
			mNavigationBarView.getDisplaycopyButton().setOnTouchListener(mDisplaycopyPreloadOnTouchListener);
		}
        updateSearchPanel();
        if("true".equals(SystemProperties.get("sys.status.hidebar_enable","false"))||mContext.getResources().getConfiguration().enableMultiWindow())
         {
            mNavigationBarView.getHidebarButton().setVisibility(View.VISIBLE);
         }else{
           mNavigationBarView.getHidebarButton().setVisibility(View.INVISIBLE);
         }
        //$_rbox_$_modify_$_huangjc,add add/remove bar button
        mNavigationBarView.getHidebarButton().setOnTouchListener(mHidebarPreloadOnTouchListener);
    
    //haungjc:win bar    
    if(mContext.getResources().getConfiguration().enableMultiWindow()){
        mNavigationBarView.getWinStartButton().setOnTouchListener(mWinStartOnTouchListener);

			//haungjc:win bar			
			mAppsScrollView = mNavigationBarView.getAppsScrollView();
			mAppsScrollView.setHorizontalScrollBarEnabled(false);//
			mAppsGridView = mNavigationBarView.getAppsGridView();
			//LOGD("wintask ====showNav====");
			 mAppsScrollView.setOnTouchListener(new View.OnTouchListener() {
				@Override
				public boolean onTouch(View v, MotionEvent event) {
				  Log.d("hjc","====MotionEvent:"+event.toString()); 
					if (popupWindow != null){
						LOGD("wintask ==mAppsScrollView== dismiss==");
									  popupWindow.dismiss();
									  mIsShowCloseApp = false;
									}
								   if (mLockpopupWindow != null) {	
									   mLockpopupWindow.dismiss();
									   mIsShowCloseApp = false;
									}
								   if(isServiceRunning("com.android.winstart.ManderService")){ 
					Intent mIntent = new Intent();
					mIntent.setAction("com.android.WINSTART");
					mIntent.setPackage("com.android.winstart");
					mContext.stopService(mIntent);
				  }
					return false;
				}});
		if(mAppsGridView !=null){
			if(appslist.size()>0){
				LOGD("wintask:reload mAppsGridView");
				listadapter=new SimpleAdapter(mContext, appslist, R.layout.gridview_item, new String[]{"icon","packagename"}, new int []{R.id.mAppsImage,R.id.mAppsText}); 
						listadapter.setViewBinder(new ViewBinder(){ 
						public boolean setViewValue(View view,Object data,String textRepresentation){ 
						if(view instanceof ImageView && data instanceof Drawable){ 
						ImageView iv=(ImageView)view; 
						iv.setImageDrawable((Drawable)data); 
						return true; 
						} 
						else 
						return false; 
						} 
						});
						
						mAppsGridView.setAdapter(listadapter);//
						LinearLayout.LayoutParams params;
						if(mDisplayMetrics.densityDpi > 240){
		                     params = new LinearLayout.LayoutParams(listadapter.getCount() * (130+0),
						 	LayoutParams.WRAP_CONTENT);
						 mAppsGridView.setColumnWidth(130); 
		                 }else if(mDisplayMetrics.densityDpi > 160){
		                     params = new LinearLayout.LayoutParams(listadapter.getCount() * (100+0),
						 	LayoutParams.WRAP_CONTENT);
						 mAppsGridView.setColumnWidth(100); 
		                 }else{
		                     params = new LinearLayout.LayoutParams(listadapter.getCount() * (70),
		                                        LayoutParams.WRAP_CONTENT);
		                                 mAppsGridView.setColumnWidth(70);
		                 }
										 mAppsGridView.setLayoutParams(params);
								 mAppsGridView.setHorizontalSpacing(0);
								 mAppsGridView.setStretchMode(GridView.NO_STRETCH);
								 //mAppsGridView.setSelector(R.drawable.bg_item_taskbar);
						 mAppsGridView.setNumColumns(listadapter.getCount());

			}
			else {
               ClearRunningTasks();
			}
			mAppsGridView.setOnItemClickListener(mWinItemClickListener);
            mAppsGridView.setOnItemLongClickListener(mWinItemLongClickListener);
			mAppsGridView.setOnGenericMotionListener(mWinItemOnGenericMotionListener);
			
		    }
		   }
    }
    
//<!-- $_rbox_$_modify_$_huangjc -->
    public void ClearRunningTasks(){
       ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
       List<ActivityManager.RecentTaskInfo> run = am.getRecentTasks(512, ActivityManager.RECENT_IGNORE_UNAVAILABLE);
       PackageManager pm =mContext.getPackageManager();
       try {
        for(ActivityManager.RecentTaskInfo ra : run){
            Intent intent = new Intent(ra.baseIntent);
            if((isCurrentHomeActivity(intent.getComponent().getPackageName(), null))||(intent.getComponent().getPackageName().equals("com.android.launcher")) || (intent.getComponent().getPackageName().equals("xxxx.xxxx.xxx"))
){ // 
                continue;
            }


            int persistentId = ra.persistentId; // pid 
            //Log.v(TAG,"kill name="+intent.getComponent().getPackageName()+"kill persistentId=" + persistentId);
            am.removeTask(persistentId/*, ActivityManager.REMOVE_TASK_KILL_PROCESS*/);
			Toast.makeText(mContext, mContext.getResources().getString(R.string.cleartasks_msg)
, 500).show();
        }
    } catch (Exception e) {
        e.printStackTrace();
    }
    }

	public boolean isCurrentHomeActivity(String component, ActivityInfo homeInfo) {
			if (homeInfo == null) {
				final PackageManager pm = mContext.getPackageManager();
				homeInfo = new Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_HOME)
					.resolveActivityInfo(pm, 0);
			}
			return homeInfo != null
				&& homeInfo.packageName.equals(component);
		}	
	 public boolean isLauncherNow(ActivityInfo homeInfo) {
	 	  String component = null;
			if (homeInfo == null) {
				final PackageManager pm = mContext.getPackageManager();
				ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
				homeInfo = new Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_HOME)
					.resolveActivityInfo(pm, 0);
				component = am.getRunningTasks(1).get(0).topActivity.getPackageName();
				//LOGD("wintask =====sLauncher:"+component);
			}
			return homeInfo != null
				&& homeInfo.packageName.equals(component);
		}
		
    public String getCurrentApps(){
        ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
		PackageManager pm = mContext.getApplicationContext().getPackageManager();
        List<ActivityManager.RecentTaskInfo> appTask = am.getRecentTasks(1,ActivityManager.RECENT_WITH_EXCLUDED|ActivityManager.RECENT_IGNORE_UNAVAILABLE);
        if(!appTask.isEmpty()){
			ActivityManager.RecentTaskInfo info = appTask.get(0);
				 
			if(info.topOfLauncher == 0) return null;
			Intent intent  = new Intent(info.baseIntent);
			if(info.origActivity != null) intent.setComponent(info.origActivity);
			ResolveInfo resolveInfo = pm.resolveActivity( intent,0);
            //      ComponentName cn = /*am.getRunningTasks(1)*/appTask.get(0).baseIntent;//topActivity;
            //      String packageName = cn.getPackageName();
            String packageName = null;
            if(resolveInfo != null)
               packageName = resolveInfo.activityInfo.packageName;
            LOGD("wintask ====getCurrentApps:"+packageName);
           return packageName;
         }
          return null;
	}
	public int getCurrentApps(String packageName){
        ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
		PackageManager pm = mContext.getApplicationContext().getPackageManager();
        List<ActivityManager.RecentTaskInfo> appTask = am.getRecentTasks(30,ActivityManager.RECENT_WITH_EXCLUDED|ActivityManager.RECENT_IGNORE_UNAVAILABLE);
        if(!appTask.isEmpty()){
			for(int i = 0;i<appTask.size();i++){
			 	ActivityManager.RecentTaskInfo info = appTask.get(i);				
				
				Intent intent  = new Intent(info.baseIntent);
				if(info.origActivity != null) intent.setComponent(info.origActivity);
				ResolveInfo resolveInfo = pm.resolveActivity( intent,0);
	         
	            String packName = null;
	            if(resolveInfo != null)
	               packName = resolveInfo.activityInfo.packageName;
				if(packageName.equals(packName)){
					LOGD(info.id+"wintask ====getCurrentApps:"+packageName +" " +info.affiliatedTaskId);
					am.moveTaskToFront(info.id,ActivityManager.MOVE_TASK_WITH_HOME,null);
					return info.id;
				}
		   }
         }
          return -1;
	   }
	public void removeAppTask(String pkNmae){
        ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
		PackageManager pm = mContext.getApplicationContext().getPackageManager();
        List<ActivityManager.RecentTaskInfo> appTask = am.getRecentTasks(50,ActivityManager.RECENT_WITH_EXCLUDED|ActivityManager.RECENT_IGNORE_UNAVAILABLE);
        if(!appTask.isEmpty()){
		  try {
        for(ActivityManager.RecentTaskInfo ra : appTask){
            Intent intent = new Intent(ra.baseIntent);
            if((isCurrentHomeActivity(intent.getComponent().getPackageName(), null))){ 
            	 
                continue;
            }

           if(intent.getComponent().getPackageName().equals(pkNmae)){
            int persistentId = ra.persistentId; // pid 
            //Log.v(TAG,"kill name="+intent.getComponent().getPackageName()+"kill persistentId=" + persistentId);
            am.removeTask(persistentId/*, ActivityManager.REMOVE_TASK_KILL_PROCESS*/);
          }
        }
    } catch (Exception e) {
        e.printStackTrace();
    }
         }
	}
	
    public boolean isServiceRunning(String SERVICE_NAME) {	
	ActivityManager manager = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
		for (RunningServiceInfo service : manager
				.getRunningServices(Integer.MAX_VALUE)) {
			if (SERVICE_NAME.equals(service.service.getClassName())) {
				return true;
			}
		}
		return false;
	}
	
	/** Returns the top task. */
    public Drawable WingetActivityIcon() {
    	 PackageManager pm = mContext.getApplicationContext().getPackageManager();
    	 ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RecentTaskInfo> appTask = am.getRecentTasks(Integer.MAX_VALUE, 1);
        Drawable icon ;
        ActivityInfo info = null;
        if (!appTask.isEmpty()) {
        	try {
            info = pm.getActivityInfo(appTask.get(0).baseIntent.getComponent(), PackageManager.GET_META_DATA);
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }  
         if(info!=null){
        	icon = info.loadIcon(pm);
            return icon;
          }
          
        }
        return (mContext.getResources().getDrawable(R.drawable.ic_launcher_vedio));
        
    }
	
	//haungjc:win bar
	public void UpdateAppsList(String pkName){
		if(mContext.getResources().getConfiguration().enableMultiWindow()){
         //ArrayList<HashMap<String, Object>> appslist=new ArrayList<HashMap<String, Object>>(); 
         LOGD("wintask UpdateAppsList,appslist bdeforce====:"+appslist.size());
		PackageManager pm = mContext.getApplicationContext().getPackageManager();
		if(isCurrentHomeActivity(pkName,null) ||"com.android.inputmethod.latin".equals(pkName)||"com.android.packageinstaller".equals(pkName) ||
					"com.rockchip.projectx".equals(pkName)|| "com.android.systemui".equals(pkName)){
			return;
			}
			
                   String getCurrentApps = getCurrentApps();	
		if(appslist.size() >0){
           Iterator<HashMap<String,Object>> it= appslist.iterator();
                HashMap<String,Object> hash;
                Iterator<String> set;
				
                for(int i = 0;i < appslist.size();i++){
                        hash=it.next();
                        set=hash.keySet().iterator();
						
                        if(set.hasNext()){
				String packageName=(String) hash.get("packagename");//get from map 
				LOGD("wintask === i ="+i+" ======hash.get packagename :"+packageName+"===pkName:"+pkName);
				if(packageName.equals(pkName)||(getCurrentApps!=null&&packageName.equals(getCurrentApps)&&isWinShow)){
				LOGD("wintask ====map already has:"+packageName);
				if(isWinShow){
					if(mAppsGridView !=null)
		                                   mAppsGridView.setSelection(i);
				}else {                        
					if(!pkName.equals(getCurrentApps)){
					LOGD("wintask ==finsh app,update list now");
					appslist.remove(i);  
                                        listadapter.notifyDataSetChanged();
					}
				}
					return;
				}
									
                                
                        }
                }
		}
	    if(!isWinShow){
			LOGD("wintask ===back press finish pkName:"+pkName);
	    	return;
	    	}
	  		//not allow add for launcher
		if(isLauncherNow(null)||(getCurrentApps!=null && isCurrentHomeActivity(getCurrentApps,null))){
			 try {
													                    Thread.sleep(200);
													                } catch (Exception ex) {
													                  //Log.e(TAG,"Exception:" +ex.getMessage());
													                }
			  if(isCurrentHomeActivity(getCurrentApps,null))
			      return;
			}
			
		HashMap<String, Object> map=new HashMap<String, Object>(); 
            //make sure some apk no add
            if(getCurrentApps==null)
                 return;
            if("com.android.inputmethod.latin".equals(getCurrentApps)||"com.android.packageinstaller".equals(getCurrentApps)){
                        return;
            }

        map.put("icon", WingetActivityIcon());
			//}
		
		map.put("packagename", getCurrentApps.toString());//
		appslist.add(map); 
	LOGD("wintask UpdateAppsList,packageName:"+(String) map.get("packagename"));
	LOGD("wintask UpdateAppsList,appslist:"+appslist.size());
		listadapter=new SimpleAdapter(mContext, appslist, R.layout.gridview_item, new String[]{"icon","packagename"}, new int []{R.id.mAppsImage,R.id.mAppsText}); 
		listadapter.setViewBinder(new ViewBinder(){ 
		public boolean setViewValue(View view,Object data,String textRepresentation){ 
		if(view instanceof ImageView && data instanceof Drawable){ 
		ImageView iv=(ImageView)view; 
		iv.setImageDrawable((Drawable)data); 
		return true; 
		} 
		else 
		return false; 
		} 
		});
		if(mAppsGridView !=null){
		mAppsGridView.setAdapter(listadapter);//
		LinearLayout.LayoutParams params;
                 if(mDisplayMetrics.densityDpi > 240){
                     params = new LinearLayout.LayoutParams(listadapter.getCount() * (130+0),
				 	LayoutParams.WRAP_CONTENT);
				 mAppsGridView.setColumnWidth(130); 
                 }else if(mDisplayMetrics.densityDpi > 160){
                     params = new LinearLayout.LayoutParams(listadapter.getCount() * (100+0),
				 	LayoutParams.WRAP_CONTENT);
				 mAppsGridView.setColumnWidth(100); 
                 }else{
                     params = new LinearLayout.LayoutParams(listadapter.getCount() * (70),
                                        LayoutParams.WRAP_CONTENT);
                                 mAppsGridView.setColumnWidth(70);
                 }
		                 mAppsGridView.setLayoutParams(params);
				 mAppsGridView.setHorizontalSpacing(0);
				 mAppsGridView.setStretchMode(GridView.NO_STRETCH);
				 //mAppsGridView.setSelector(R.drawable.bg_item_taskbar);
		 mAppsGridView.setNumColumns(listadapter.getCount());
			}
		
	}
} 
private boolean mIsShowCloseApp = false;
private boolean mIsButton1Lock = false;
private boolean mIsButton2Lock = false;

private boolean	mRightMouseClick = false;
private String appclosename = null;
	public void showPopuWindow(int x,String name) {  
        ////if (popupWindow == null) { 
        appclosename = name;
         if (popupWindow != null)  
			popupWindow.dismiss();
		    WindowManager wm = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
            LayoutInflater layoutInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);  
  
            View popupview = layoutInflater.inflate(R.layout.winpopmenu, null);  
           // ImageView mPopupIcon = (ImageView) popupview.findViewById(R.id.popupicon);
			Button mPopupText = (Button) popupview.findViewById(R.id.popuptext);
			mPopupText.setText(R.string.popuwindow_close_app);
			mPopupText.setClickable(true);
			mPopupText.setOnClickListener(new Button.OnClickListener(){//
            public void onClick(View v) {    
                 //Toast.makeText(mContext,"close the app", 1000).show();
                 if(appclosename!=null){
				 	removeAppTask(appclosename);
				    isWinShow = false;
			         UpdateAppsList(appclosename);
				  }
						   if (popupWindow != null) {  
						   	LOGD("wintask ==showPopuWindow dismiss==");
							   popupWindow.dismiss();
							   mIsShowCloseApp = false;
						   }     
            }    
  
        });     
            popupWindow = new Dialog(mContext,R.style.FullHeightDialog);
		   popupWindow.getWindow().setGravity(Gravity.LEFT/* | Gravity.BOTTOM*/);
			wmDParams = popupWindow.getWindow().getAttributes();
			wmDParams.width = 250;		
			wmDParams.height =100;
      if(mDisplayMetrics.densityDpi > 240){
			wmDParams.x = x-45;
			wmDParams.y = wm.getDefaultDisplay().getHeight()-810;
			}else if(mDisplayMetrics.densityDpi > 160){
			wmDParams.x = x-30;
			wmDParams.y = wm.getDefaultDisplay().getHeight()-400;
			}else{
			wmDParams.x = x-25;
      wmDParams.y = wm.getDefaultDisplay().getHeight()-200;
      }
			wmDParams.format = 1;
			popupWindow.setContentView(popupview);
			popupWindow.getWindow().setAttributes(wmDParams);
			popupWindow.getWindow().setType(2002);
            popupWindow.setCanceledOnTouchOutside(true); 

		    popupWindow.show();
	        mIsShowCloseApp = true;
		}

    public void showLockPopuWindow(int x,String name) {  
      if(mContext.getResources().getConfiguration().enableMultiWindow()){    
        appclosename = name;
         if (mLockpopupWindow != null)  
			mLockpopupWindow.dismiss();
		    WindowManager wm = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
            LayoutInflater layoutInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);  
  
            View popupview = layoutInflater.inflate(R.layout.winpopmenu, null);  
			Button mPopupText = (Button) popupview.findViewById(R.id.popuptext);
			mPopupText.setText(R.string.popuwindow_lock_app);
			mPopupText.setTextSize(12);
			mPopupText.setClickable(true);
			mPopupText.setOnClickListener(new Button.OnClickListener(){//
            public void onClick(View v) {    
                 //Toast.makeText(mContext,"close the app", 1000).show();
					   if (mLockpopupWindow != null) {  
						   mLockpopupWindow.dismiss();
						   mIsShowCloseApp = false;
						   }     
            }    
  
        });    
						   				  
            mLockpopupWindow = new Dialog(mContext,R.style.FullHeightDialog);
		   mLockpopupWindow.getWindow().setGravity(Gravity.LEFT );
			wmDParams = mLockpopupWindow.getWindow().getAttributes();
		    
			wmDParams.width = 300;		
			wmDParams.height =100;
			if(mDisplayMetrics.densityDpi > 160){
			wmDParams.x = x-45;
			wmDParams.y = wm.getDefaultDisplay().getHeight()-810;
			}else{
			wmDParams.x = x-25;
      wmDParams.y = wm.getDefaultDisplay().getHeight()-200;
      }
			wmDParams.format = 1;
			mLockpopupWindow.setContentView(popupview);
			mLockpopupWindow.getWindow().setAttributes(wmDParams);
			mLockpopupWindow.getWindow().setType(2002);
		
       // }
     
        mLockpopupWindow.setCanceledOnTouchOutside(true); 

		    mLockpopupWindow.show();
	        mIsShowCloseApp = true;
	      }
		}
	
	public void ForceStopRunningApp(String packagename){
        ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
                  Log.d(TAG," we forceStopPackage :"+packagename);
                  am.forceStopPackage(packagename);
	}

    private AdapterView.OnItemClickListener mWinItemClickListener = new AdapterView.OnItemClickListener() {
             @Override
			public void onItemClick(AdapterView<?> parent, View view, int position, 
				long id) { 
			if(mContext.getResources().getConfiguration().enableMultiWindow()){
				//get the item of the list to a hashmap 
				HashMap<?, ?> map=(HashMap<?, ?>)parent.getItemAtPosition(position); 
				//get package name from map 
				String packageName=(String) map.get("packagename");//get from map 
				//if we onclick the item then start the application 
				LOGD("wintask onItemClick,packageName:"+packageName);				
				   if (mLockpopupWindow != null) {  
					   mLockpopupWindow.dismiss();
					   mIsShowCloseApp = false;
				   	}
				   if(isServiceRunning("com.android.winstart.ManderService")){ 
                        Intent mIntent = new Intent();
                        mIntent.setAction("com.android.WINSTART");
                        mIntent.setPackage("com.android.winstart");
                        mContext.stopService(mIntent);
                      }
				if(!packageName.equals(getCurrentApps())/*||isLauncherNow(null)*/){
				   PackageManager pm = mContext.getApplicationContext().getPackageManager();
				   Intent intent=new Intent(); 
				   intent =pm.getLaunchIntentForPackage(packageName);
				   if(intent!=null&&!mRightMouseClick&&getCurrentApps(packageName) != -1){

				}else
				if(intent!=null&&!mRightMouseClick){
					intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_SINGLE_TOP );
					mContext.startActivity(intent);
				}
				mRightMouseClick = false;
				//doStartApplicationWithPackageName(packageName);
					}
				}
             	}
			 };
	private AdapterView.OnItemLongClickListener mWinItemLongClickListener = new AdapterView.OnItemLongClickListener() {
             @Override
			 public boolean onItemLongClick(AdapterView<?> parent, View view, 
				int position, long id) { 
				HashMap<?, ?> long_map=(HashMap<?, ?>)parent.getItemAtPosition(position); 
				String packageName=(String)long_map.get("packagename"); 
				int[] location = new  int[2] ;
                view.getLocationOnScreen(location);
				showPopuWindow(location [0]+10,packageName);
				return true;
				}
			 };
	
		private View.OnGenericMotionListener mWinItemOnGenericMotionListener = new View.OnGenericMotionListener() {
                        
                        @Override
                        public boolean onGenericMotion(View v, MotionEvent event) {
                                // TODO Auto-generated method stub
               if(mContext.getResources().getConfiguration().enableMultiWindow()){             
								if ((event.getSource() & InputDevice.SOURCE_CLASS_POINTER) != 0) {
                                int what = event.getButtonState();
								float currentXPosition = event.getX();  
                                float currentYPosition = event.getY();  
                                int position = mAppsGridView.pointToPosition((int) currentXPosition, (int) currentYPosition );
								switch(what){
								case MotionEvent.BUTTON_SECONDARY:
									//Toast.makeText(mContext, "mouse for right click", 300).show();
									mRightMouseClick = true;
									if(position != -1){
										HashMap<?, ?> map=(HashMap<?, ?>)mAppsGridView.getItemAtPosition(position); 
				                        //get package name from map 
				                        String packageName=(String) map.get("packagename");//get from map 
									showPopuWindow((int)event.getRawX()-30,packageName);
										}
								     break;
								
								case MotionEvent.BUTTON_PRIMARY:
									//Toast.makeText(mContext, "mouse for left click", 300).show();

								    break;
									}
								return true;
									
									}
								}
                                return false;
                        	}
                };
	
    private View.OnTouchListener mWinStartOnTouchListener = new View.OnTouchListener() {

                @Override
                public boolean onTouch(View v, MotionEvent event) {
               if(mContext.getResources().getConfiguration().enableMultiWindow()){
                        // TODO Auto-generated method stub
                         int[] location = new int[2];
                     int action = event.getAction() & MotionEvent.ACTION_MASK;
            if (action == MotionEvent.ACTION_DOWN) {
				if (popupWindow != null){  
				    popupWindow.dismiss();
				    mIsShowCloseApp = false;
				}
				if (mLockpopupWindow != null) {  
				    mLockpopupWindow.dismiss();
				    mIsShowCloseApp = false;
				}
            } else if (action == MotionEvent.ACTION_CANCEL) {

            } else if (action == MotionEvent.ACTION_UP) {
                    if(!isServiceRunning("com.android.winstart.ManderService")){ 
                        Intent mIntent = new Intent();
                        mIntent.setAction("com.android.WINSTART");
                        mIntent.setPackage("com.android.winstart");
                        mContext.startService(mIntent);
                      }else{
                        Intent mIntent = new Intent();
                        mIntent.setAction("com.android.WINSTART");
                        mIntent.setPackage("com.android.winstart");
                        mContext.stopService(mIntent);
                      }
                        
            }
              }
                        return false;
                }
        };

    private View.OnTouchListener mDisplaycopyPreloadOnTouchListener = new View.OnTouchListener() {
	// additional optimization when we have software system buttons - start loading the recent
	// tasks on touch down
	@Override
	public boolean onTouch(View v, MotionEvent event) {
		int action = event.getAction() & MotionEvent.ACTION_MASK;
		if (action == MotionEvent.ACTION_DOWN) {
		} else if (action == MotionEvent.ACTION_CANCEL) {
		} else if (action == MotionEvent.ACTION_UP) {
			Log.d("SystemUI","onTouch display copy ");
			int display_sync = Settings.System.getInt(mContext.getContentResolver(),
				Settings.System.DISPLAY_SHOW_SYNCHRONIZATION,0);
			Settings.System.putInt(mContext.getContentResolver(),
				Settings.System.DISPLAY_SHOW_SYNCHRONIZATION, (display_sync + 1) % 2);
		}
		return false;
	}
    };

    private final String Dual_Screen_KEY = Settings.System.DUAL_SCREEN_MODE;
    private final String Dual_Screen_Icon_used_KEY = Settings.System.DUAL_SCREEN_ICON_USED;
    private final ContentObserver mDualScreenValueObserver = new ContentObserver(new Handler()) {
	@Override
	public void onChange(boolean selfChange) {
		final boolean enable = getDualScreenValue() != 0;
		if(enable){
			mNavigationBarView.getDisplaycopyButton().setVisibility(View.VISIBLE);
		} else {
			mNavigationBarView.getDisplaycopyButton().setVisibility(View.GONE);
		}
	}
    };

    private final ContentObserver mDualScreenIconUsedObserver = new ContentObserver(new Handler()) {
	@Override
	public void onChange(boolean selfChange) {
		final boolean enable = getDualScreenIconUsedValue() != 0;
		if(enable){
			mNavigationBarView.getDisplaycopyButton().setVisibility(View.VISIBLE);
		} else {
			mNavigationBarView.getDisplaycopyButton().setVisibility(View.GONE);
		}
	}
    };

    private int getDualScreenValue(){
	return Settings.System.getInt(mContext.getContentResolver(), Dual_Screen_KEY, 0);
    }

    private int getDualScreenIconUsedValue(){
	return Settings.System.getInt(mContext.getContentResolver(), Dual_Screen_Icon_used_KEY, 0);
    }
	
//huangjc:showSingleChoiceButton
    private AlertDialog builder;	 
    private void showSingleChoiceButton(){
         String title = mContext.getResources()
            .getString(R.string.config_multi_dialog_title);  
         String ok = mContext.getResources()
            .getString(com.android.internal.R.string.ok);
         String cancel = mContext.getResources()
            .getString(com.android.internal.R.string.cancel);
         String[] province = mContext.getResources()
            .getStringArray(R.array.config_multi_dialog_chose);
         
       MButtonOnClick buttonOnClick = new MButtonOnClick(0);
       builder = new AlertDialog.Builder(mContext)
              .setTitle(title)
              .setSingleChoiceItems(province, 0, buttonOnClick)
              .setPositiveButton(ok, buttonOnClick)
              .setNegativeButton(cancel, buttonOnClick)
              .create();
       builder.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
       builder.show();
    }	
  private class MButtonOnClick implements DialogInterface.OnClickListener
    {
       private int index;
       public MButtonOnClick(int index)
       {
           this.index = index;
       }
       @Override
       public void onClick(DialogInterface dialog,int which)
       {
           if(which >0||which == 0){
             index = which;
           }else if(which == DialogInterface.BUTTON_POSITIVE){
             switch(index){
               case 0:
                Intent intent1 = new Intent();
                intent1.setComponent(new ComponentName("com.rockchip.projectx", "com.rockchip.projectx.RegionCapture2"));
                intent1.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                mContext.startActivity(intent1);
                  break;
               case 1:
                Intent intent2 = new Intent();
                intent2.setComponent(new ComponentName("com.rockchip.projectx", "com.rockchip.projectx.RecordScreenActivity"));
                intent2.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                mContext.startActivity(intent2);
                  break;
                case 2:
                 mHandler.postDelayed(new Runnable() {
                        public void run() {
                            Intent intent0 = new Intent("rk.android.screenshot.action");
                  mContext.sendBroadcast(intent0);
                        }
                    }, 500);
                  break;
               case 3:
                  Intent intent3 = new Intent();
                intent3.setComponent(new ComponentName("com.rockchip.projectx", "com.rockchip.projectx.RegionCapture2"));
                intent3.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                intent3.putExtra("IS_FULL_SCREEN_POSTIL", true);
                mContext.startActivity(intent3);
                  break;
               default:
                 dialog.dismiss();
           }
            dialog.dismiss();
       }else {
        dialog.dismiss();
       }
    }
   }


   
    private View.OnTouchListener mScreenshotPreloadOnTouchListener = new View.OnTouchListener() {
		        // additional optimization when we have software system buttons - start loading the recent
		        // tasks on touch down
		        @Override
		        public boolean onTouch(View v, MotionEvent event) {
				            int action = event.getAction() & MotionEvent.ACTION_MASK;
				            if (action == MotionEvent.ACTION_DOWN) {
						                Log.d("dzy","onTouch screenshot ");
	               		          //   takeScreenshot();
                       if(mContext.getResources().getConfiguration().enableMultiWindow()){
                       //add by huangjc for Multi ScreenShot
                              if(builder!=null)
                                builder.dismiss();
                                showSingleChoiceButton();
                        }else {
                              Intent intent = new Intent("rk.android.screenshot.action");
						  mContext.sendBroadcast(intent);
                        }
						            } else if (action == MotionEvent.ACTION_CANCEL) {
								            } else if (action == MotionEvent.ACTION_UP) {
             }
             return false;
         }
     };
	 
     private View.OnTouchListener mPowerPreloadOnTouchListener = new View.OnTouchListener() {
		
		@Override
		public boolean onTouch(View v, MotionEvent event) {
			// TODO Auto-generated method stub
			int action = event.getAction() & MotionEvent.ACTION_MASK;
            if (action == MotionEvent.ACTION_DOWN) {
        		Intent i = new Intent();
        		i.setAction("com.firefly.systemui.poweroff");
        		mContext.sendBroadcast(i);
        		/*
            	try {
            		mWindowManagerService.shutdown();
            	} catch (RemoteException e){
            		Log.d(TAG,"shutdown error =" + e);
            	}*/
            } else if (action == MotionEvent.ACTION_CANCEL) {
            	
            } else if (action == MotionEvent.ACTION_UP) {
            	
            }		
			return false;
		}
	};		 
	 
      //$_rbox_$_modify_$_huangjc,add add/remove bar button	
      private View.OnTouchListener mHidebarPreloadOnTouchListener = new View.OnTouchListener() {

                @Override
                public boolean onTouch(View v, MotionEvent event) {
                        // TODO Auto-generated method stub
                     int action = event.getAction() & MotionEvent.ACTION_MASK;
            if (action == MotionEvent.ACTION_DOWN) {

            } else if (action == MotionEvent.ACTION_CANCEL) {

            } else if (action == MotionEvent.ACTION_UP) {
                removeBar();
            }
                        return false;
                }
        };

        private boolean mBarIsAdd = true;
	private static final int MSG_CHANGE_BAR_HIDE_STATUS = 2030;

        private void changeBarHideStatus()
        {
            boolean hide_systembar = Settings.System.getInt(mContext.getContentResolver(),Settings.System.SYSTEMBAR_HIDE,0)==1;

            if(hide_systembar)
                removeBar(false);
            else{
                addBarInside(hide_systembar);   
            }
        }

        private void addBarInside(){
            boolean hide_systembar = Settings.System.getInt(mContext.getContentResolver(),Settings.System.SYSTEMBAR_HIDE,0)==1;
            addBarInside(hide_systembar);
        }

        private void addBarInside(boolean alwaysHide){
            if (!mBarIsAdd && !alwaysHide){
                Log.d(TAG,"add Bar");
            
          try {
            boolean showNav = mWindowManagerService.hasNavigationBar();
            if (DEBUG) Log.v(TAG, "hasNavigationBar=" + showNav);
            if (showNav) {
            	//haungjc:win bar
            	if(mContext.getResources().getConfiguration().enableMultiWindow()){
					//$_rockchip_$_modify_$_huangjc begin,add show/hide TitleBar interface for statusbar
					try {
			            mWindowManagerService.changeTitleBar(true);
			        } catch (RemoteException e) {
			            Log.w(TAG, "Error changeTitleBar transition: " + e);
			        }
					//$_rockchip_$_modify_$_end
            		mNavigationBarView =
                    (NavigationBarView) View.inflate(mContext, R.layout.navigation_bar_win, null);
              }else{
                mNavigationBarView =
                    (NavigationBarView) View.inflate(mContext, R.layout.navigation_bar, null);
              }
                mNavigationBarView.setDisabledFlags(mDisabled);
                mNavigationBarView.setBar(this);
                mNavigationBarView.setOnVerticalChangedListener(
                        new NavigationBarView.OnVerticalChangedListener() {
                    @Override
                    public void onVerticalChanged(boolean isVertical) {
                        if (mSearchPanelView != null) {
                            mSearchPanelView.setHorizontal(isVertical);
                        }
                        mNotificationPanel.setQsScrimEnabled(!isVertical);
                    }
                });
                mNavigationBarView.setOnTouchListener(new View.OnTouchListener() {
                    @Override
                    public boolean onTouch(View v, MotionEvent event) {
                        checkUserAutohide(v, event);
                        return false;
                    }});
            }
        } catch (RemoteException ex) {
            // no window manager? good luck with that
        }
                        addNavigationBar();
            if (mNavigationBarView != null)
            {
                setInteracting(StatusBarManager.WINDOW_STATUS_BAR, true);
            }
            if (mStatusBarWindow != null)
                mStatusBarWindow.setVisibility(View.VISIBLE);
                mBarIsAdd = true;
            }
        }

        private void removeBar(){
            removeBar(true);
        }

        private void removeBar(boolean needToast){
                if (mBarIsAdd){
                        Log.d(TAG,"remove Bar");

					//$_rockchip_$_modify_$_huangjc begin,add show/hide TitleBar interface for statusbar
					if(mContext.getResources().getConfiguration().enableMultiWindow()){
						try {
				            mWindowManagerService.changeTitleBar(false);
				        } catch (RemoteException e) {
				            Log.w(TAG, "Error changeTitleBar transition: " + e);
				        }
						}
					//$_rockchip_$_modify_$_end
                        if (mNavigationBarView != null)
                                mWindowManager.removeViewImmediate(mNavigationBarView);
                       
                       if (mStatusBarWindow != null)
					 	  mStatusBarWindow.setVisibility(View.GONE);
                        mBarIsAdd = false;
                        if(!isMultiChange)
                        Toast.makeText(mContext, mContext.getResources().getString(R.string.hidebar_msg)
, 1000).show();
                }
        }

        @Override // CommandQueue
        public void addBar(){
                addBarInside();
        }
        //$_rbox_$_modify_$_huangjc end

    // For small-screen devices (read: phones) that lack hardware navigation buttons
    private void addNavigationBar() {
        if (DEBUG) Log.v(TAG, "addNavigationBar: about to add " + mNavigationBarView);
        if (mNavigationBarView == null) return;

        prepareNavigationBarView();

        mWindowManager.addView(mNavigationBarView, getNavigationBarLayoutParams());
    }

    private void repositionNavigationBar() {
        if (mNavigationBarView == null || !mNavigationBarView.isAttachedToWindow()) return;

        prepareNavigationBarView();
        mWindowManager.updateViewLayout(mNavigationBarView, getNavigationBarLayoutParams());
    }

    private void notifyNavigationBarScreenOn(boolean screenOn) {
        if (mNavigationBarView == null) return;
        mNavigationBarView.notifyScreenOn(screenOn);
    }

    private WindowManager.LayoutParams getNavigationBarLayoutParams() {
        WindowManager.LayoutParams lp = new WindowManager.LayoutParams(
                LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.TYPE_NAVIGATION_BAR,
                    0
                    | WindowManager.LayoutParams.FLAG_TOUCHABLE_WHEN_WAKING
                    | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                    | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                    | WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH
                    | WindowManager.LayoutParams.FLAG_SPLIT_TOUCH,
                PixelFormat.TRANSLUCENT);
        // this will allow the navbar to run in an overlay on devices that support this
        if (ActivityManager.isHighEndGfx()) {
            lp.flags |= WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
        }

        lp.setTitle("NavigationBar");
        lp.windowAnimations = 0;
        return lp;
    }

    private void addHeadsUpView() {
        int headsUpHeight = mContext.getResources()
                .getDimensionPixelSize(R.dimen.heads_up_window_height);
        WindowManager.LayoutParams lp = new WindowManager.LayoutParams(
                LayoutParams.MATCH_PARENT, headsUpHeight,
                WindowManager.LayoutParams.TYPE_STATUS_BAR_PANEL, // above the status bar!
                WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                    | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
                    | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                    | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                    | WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM
                    | WindowManager.LayoutParams.FLAG_SPLIT_TOUCH,
                PixelFormat.TRANSLUCENT);
        lp.flags |= WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
        lp.gravity = Gravity.TOP;
        lp.setTitle("Heads Up");
        lp.packageName = mContext.getPackageName();
        lp.windowAnimations = R.style.Animation_StatusBar_HeadsUp;

        mWindowManager.addView(mHeadsUpNotificationView, lp);
    }

    private void removeHeadsUpView() {
        mWindowManager.removeView(mHeadsUpNotificationView);
    }

    public void refreshAllStatusBarIcons() {
        refreshAllIconsForLayout(mStatusIcons);
        refreshAllIconsForLayout(mStatusIconsKeyguard);
        refreshAllIconsForLayout(mNotificationIcons);
    }

    private void refreshAllIconsForLayout(LinearLayout ll) {
        final int count = ll.getChildCount();
        for (int n = 0; n < count; n++) {
            View child = ll.getChildAt(n);
            if (child instanceof StatusBarIconView) {
                ((StatusBarIconView) child).updateDrawable();
            }
        }
    }

    public void addIcon(String slot, int index, int viewIndex, StatusBarIcon icon) {
        if (SPEW) Log.d(TAG, "addIcon slot=" + slot + " index=" + index + " viewIndex=" + viewIndex
                + " icon=" + icon);
        StatusBarIconView view = new StatusBarIconView(mContext, slot, null);
        view.set(icon);
        mStatusIcons.addView(view, viewIndex, new LinearLayout.LayoutParams(
                LayoutParams.WRAP_CONTENT, mIconSize));
        view = new StatusBarIconView(mContext, slot, null);
        view.set(icon);
        mStatusIconsKeyguard.addView(view, viewIndex, new LinearLayout.LayoutParams(
                LayoutParams.WRAP_CONTENT, mIconSize));
    }

    public void updateIcon(String slot, int index, int viewIndex,
            StatusBarIcon old, StatusBarIcon icon) {
        if (SPEW) Log.d(TAG, "updateIcon slot=" + slot + " index=" + index + " viewIndex=" + viewIndex
                + " old=" + old + " icon=" + icon);
        StatusBarIconView view = (StatusBarIconView) mStatusIcons.getChildAt(viewIndex);
        view.set(icon);
        view = (StatusBarIconView) mStatusIconsKeyguard.getChildAt(viewIndex);
        view.set(icon);
    }

    public void removeIcon(String slot, int index, int viewIndex) {
        if (SPEW) Log.d(TAG, "removeIcon slot=" + slot + " index=" + index + " viewIndex=" + viewIndex);
        mStatusIcons.removeViewAt(viewIndex);
        mStatusIconsKeyguard.removeViewAt(viewIndex);
    }

    public UserHandle getCurrentUserHandle() {
        return new UserHandle(mCurrentUserId);
    }

    @Override
    public void addNotification(StatusBarNotification notification, RankingMap ranking) {
        if (DEBUG) Log.d(TAG, "addNotification key=" + notification.getKey());
        if (mUseHeadsUp && shouldInterrupt(notification)) {
            if (DEBUG) Log.d(TAG, "launching notification in heads up mode");
            Entry interruptionCandidate = new Entry(notification, null);
            ViewGroup holder = mHeadsUpNotificationView.getHolder();
            if (inflateViewsForHeadsUp(interruptionCandidate, holder)) {
                // 1. Populate mHeadsUpNotificationView
                mHeadsUpNotificationView.showNotification(interruptionCandidate);

                // do not show the notification in the shade, yet.
                return;
            }
        }

        Entry shadeEntry = createNotificationViews(notification);
        if (shadeEntry == null) {
            return;
        }

        if (notification.getNotification().fullScreenIntent != null) {
            // Stop screensaver if the notification has a full-screen intent.
            // (like an incoming phone call)
            awakenDreams();

            // not immersive & a full-screen alert should be shown
            if (DEBUG) Log.d(TAG, "Notification has fullScreenIntent; sending fullScreenIntent");
            try {
                EventLog.writeEvent(EventLogTags.SYSUI_FULLSCREEN_NOTIFICATION,
                        notification.getKey());
                notification.getNotification().fullScreenIntent.send();
            } catch (PendingIntent.CanceledException e) {
            }
        } else {
            // usual case: status bar visible & not immersive

            // show the ticker if there isn't already a heads up
            if (mHeadsUpNotificationView.getEntry() == null) {
                tick(notification, true);
            }
        }
        addNotificationViews(shadeEntry, ranking);
        // Recalculate the position of the sliding windows and the titles.
        setAreThereNotifications();
        updateExpandedViewPos(EXPANDED_LEAVE_ALONE);
    }

    public void displayNotificationFromHeadsUp(StatusBarNotification notification) {
        NotificationData.Entry shadeEntry = createNotificationViews(notification);
        if (shadeEntry == null) {
            return;
        }
        shadeEntry.setInterruption();

        addNotificationViews(shadeEntry, null);
        // Recalculate the position of the sliding windows and the titles.
        setAreThereNotifications();
        updateExpandedViewPos(EXPANDED_LEAVE_ALONE);
    }

    @Override
    public void resetHeadsUpDecayTimer() {
        mHandler.removeMessages(MSG_DECAY_HEADS_UP);
        if (mUseHeadsUp && mHeadsUpNotificationDecay > 0
                && mHeadsUpNotificationView.isClearable()) {
            mHandler.sendEmptyMessageDelayed(MSG_DECAY_HEADS_UP, mHeadsUpNotificationDecay);
        }
    }

    @Override
    public void scheduleHeadsUpOpen() {
        mHandler.removeMessages(MSG_SHOW_HEADS_UP);
        mHandler.sendEmptyMessage(MSG_SHOW_HEADS_UP);
    }

    @Override
    public void scheduleHeadsUpClose() {
        mHandler.removeMessages(MSG_HIDE_HEADS_UP);
        mHandler.sendEmptyMessage(MSG_HIDE_HEADS_UP);
    }

    @Override
    public void scheduleHeadsUpEscalation() {
        mHandler.removeMessages(MSG_ESCALATE_HEADS_UP);
        mHandler.sendEmptyMessage(MSG_ESCALATE_HEADS_UP);
    }

    @Override
    protected void updateNotificationRanking(RankingMap ranking) {
        mNotificationData.updateRanking(ranking);
        updateNotifications();
    }

    @Override
    public void removeNotification(String key, RankingMap ranking) {
        if (ENABLE_HEADS_UP && mHeadsUpNotificationView.getEntry() != null
                && key.equals(mHeadsUpNotificationView.getEntry().notification.getKey())) {
            mHeadsUpNotificationView.clear();
        }

        StatusBarNotification old = removeNotificationViews(key, ranking);
        if (SPEW) Log.d(TAG, "removeNotification key=" + key + " old=" + old);

        if (old != null) {
            // Cancel the ticker if it's still running
            if (mTickerEnabled) {
                mTicker.removeEntry(old);
            }

            // Recalculate the position of the sliding windows and the titles.
            updateExpandedViewPos(EXPANDED_LEAVE_ALONE);

            if (CLOSE_PANEL_WHEN_EMPTIED && !hasActiveNotifications()
                    && !mNotificationPanel.isTracking() && !mNotificationPanel.isQsExpanded()) {
                if (mState == StatusBarState.SHADE) {
                    animateCollapsePanels();
                } else if (mState == StatusBarState.SHADE_LOCKED) {
                    goToKeyguard();
                }
            }
        }
        setAreThereNotifications();
    }

    @Override
    protected void refreshLayout(int layoutDirection) {
        if (mNavigationBarView != null) {
            mNavigationBarView.setLayoutDirection(layoutDirection);
        }
        refreshAllStatusBarIcons();
    }

    private void updateShowSearchHoldoff() {
        mShowSearchHoldoff = mContext.getResources().getInteger(
            R.integer.config_show_search_delay);
    }

    private void updateNotificationShade() {
        if (mStackScroller == null) return;

        // Do not modify the notifications during collapse.
        if (isCollapsing()) {
            addPostCollapseAction(new Runnable() {
                @Override
                public void run() {
                    updateNotificationShade();
                }
            });
            return;
        }

        ArrayList<Entry> activeNotifications = mNotificationData.getActiveNotifications();
        ArrayList<ExpandableNotificationRow> toShow = new ArrayList<>(activeNotifications.size());
        final int N = activeNotifications.size();
        for (int i=0; i<N; i++) {
            Entry ent = activeNotifications.get(i);
            int vis = ent.notification.getNotification().visibility;

            // Display public version of the notification if we need to redact.
            final boolean hideSensitive =
                    !userAllowsPrivateNotificationsInPublic(ent.notification.getUserId());
            boolean sensitiveNote = vis == Notification.VISIBILITY_PRIVATE;
            boolean sensitivePackage = packageHasVisibilityOverride(ent.notification.getKey());
            boolean sensitive = (sensitiveNote && hideSensitive) || sensitivePackage;
            boolean showingPublic = sensitive && isLockscreenPublicMode();
            ent.row.setSensitive(sensitive);
            if (ent.autoRedacted && ent.legacy) {
                // TODO: Also fade this? Or, maybe easier (and better), provide a dark redacted form
                // for legacy auto redacted notifications.
                if (showingPublic) {
                    ent.row.setShowingLegacyBackground(false);
                } else {
                    ent.row.setShowingLegacyBackground(true);
                }
            }
            toShow.add(ent.row);
        }

        ArrayList<View> toRemove = new ArrayList<View>();
        for (int i=0; i< mStackScroller.getChildCount(); i++) {
            View child = mStackScroller.getChildAt(i);
            if (!toShow.contains(child) && child instanceof ExpandableNotificationRow) {
                toRemove.add(child);
            }
        }

        for (View remove : toRemove) {
            mStackScroller.removeView(remove);
        }
        for (int i=0; i<toShow.size(); i++) {
            View v = toShow.get(i);
            if (v.getParent() == null) {
                mStackScroller.addView(v);
            }
        }

        // So after all this work notifications still aren't sorted correctly.
        // Let's do that now by advancing through toShow and mStackScroller in
        // lock-step, making sure mStackScroller matches what we see in toShow.
        int j = 0;
        for (int i = 0; i < mStackScroller.getChildCount(); i++) {
            View child = mStackScroller.getChildAt(i);
            if (!(child instanceof ExpandableNotificationRow)) {
                // We don't care about non-notification views.
                continue;
            }

            if (child == toShow.get(j)) {
                // Everything is well, advance both lists.
                j++;
                continue;
            }

            // Oops, wrong notification at this position. Put the right one
            // here and advance both lists.
            mStackScroller.changeViewPosition(toShow.get(j), i);
            j++;
        }
        updateRowStates();
        updateSpeedbump();
        updateClearAll();
        updateEmptyShadeView();

        // Disable QS if device not provisioned.
        // If the user switcher is simple then disable QS during setup because
        // the user intends to use the lock screen user switcher, QS in not needed.
        mNotificationPanel.setQsExpansionEnabled(isDeviceProvisioned()
                && (mUserSetup || mUserSwitcherController == null
                        || !mUserSwitcherController.isSimpleUserSwitcher()));
        mShadeUpdates.check();
    }

    private boolean packageHasVisibilityOverride(String key) {
        return mNotificationData.getVisibilityOverride(key)
                != NotificationListenerService.Ranking.VISIBILITY_NO_OVERRIDE;
    }

    private void updateClearAll() {
        boolean showDismissView =
                mState != StatusBarState.KEYGUARD &&
                mNotificationData.hasActiveClearableNotifications();
        mStackScroller.updateDismissView(showDismissView);
    }

    private void updateEmptyShadeView() {
        boolean showEmptyShade =
                mState != StatusBarState.KEYGUARD &&
                        mNotificationData.getActiveNotifications().size() == 0;
        mNotificationPanel.setShadeEmpty(showEmptyShade);
    }

    private void updateSpeedbump() {
        int speedbumpIndex = -1;
        int currentIndex = 0;
        ArrayList<Entry> activeNotifications = mNotificationData.getActiveNotifications();
        final int N = activeNotifications.size();
        for (int i = 0; i < N; i++) {
            Entry entry = activeNotifications.get(i);
            if (entry.row.getVisibility() != View.GONE &&
                    mNotificationData.isAmbient(entry.key)) {
                speedbumpIndex = currentIndex;
                break;
            }
            currentIndex++;
        }
        mStackScroller.updateSpeedBumpIndex(speedbumpIndex);
    }

    @Override
    protected void updateNotifications() {
        // TODO: Move this into updateNotificationIcons()?
        if (mNotificationIcons == null) return;

        mNotificationData.filterAndSort();

        updateNotificationShade();
        updateNotificationIcons();
    }

    private void updateNotificationIcons() {
        final LinearLayout.LayoutParams params
            = new LinearLayout.LayoutParams(mIconSize + 2*mIconHPadding, mNaturalBarHeight);

        ArrayList<Entry> activeNotifications = mNotificationData.getActiveNotifications();
        final int N = activeNotifications.size();
        ArrayList<StatusBarIconView> toShow = new ArrayList<>(N);

        // Filter out notifications with low scores.
        for (int i = 0; i < N; i++) {
            Entry ent = activeNotifications.get(i);
            if (ent.notification.getScore() < HIDE_ICONS_BELOW_SCORE &&
                    !NotificationData.showNotificationEvenIfUnprovisioned(ent.notification)) {
                continue;
            }
            toShow.add(ent.icon);
        }

        if (DEBUG) {
            Log.d(TAG, "refreshing icons: " + toShow.size() +
                    " notifications, mNotificationIcons=" + mNotificationIcons);
        }

        ArrayList<View> toRemove = new ArrayList<View>();
        for (int i=0; i<mNotificationIcons.getChildCount(); i++) {
            View child = mNotificationIcons.getChildAt(i);
            if (!toShow.contains(child)) {
                toRemove.add(child);
            }
        }

        final int toRemoveCount = toRemove.size();
        for (int i = 0; i < toRemoveCount; i++) {
            mNotificationIcons.removeView(toRemove.get(i));
        }

        for (int i=0; i<toShow.size(); i++) {
            View v = toShow.get(i);
            if (v.getParent() == null) {
                mNotificationIcons.addView(v, i, params);
            }
        }

        // Resort notification icons
        final int childCount = mNotificationIcons.getChildCount();
        for (int i = 0; i < childCount; i++) {
            View actual = mNotificationIcons.getChildAt(i);
            StatusBarIconView expected = toShow.get(i);
            if (actual == expected) {
                continue;
            }
            mNotificationIcons.removeView(expected);
            mNotificationIcons.addView(expected, i);
        }
    }

    @Override
    protected void updateRowStates() {
        super.updateRowStates();
        mNotificationPanel.notifyVisibleChildrenChanged();
    }

    protected void updateCarrierLabelVisibility(boolean force) {
        // TODO: Handle this for the notification stack scroller as well
        if (!mShowCarrierInPanel) return;
        // The idea here is to only show the carrier label when there is enough room to see it,
        // i.e. when there aren't enough notifications to fill the panel.
        if (SPEW) {
            Log.d(TAG, String.format("stackScrollerh=%d scrollh=%d carrierh=%d",
                    mStackScroller.getHeight(), mStackScroller.getHeight(),
                    mCarrierLabelHeight));
        }

        // Emergency calls only is shown in the expanded header now.
        final boolean emergencyCallsShownElsewhere = true;
        final boolean makeVisible =
            !(emergencyCallsShownElsewhere && mNetworkController.isEmergencyOnly())
            && mStackScroller.getHeight() < (mNotificationPanel.getHeight()
                    - mCarrierLabelHeight - mStatusBarHeaderHeight)
            && mStackScroller.getVisibility() == View.VISIBLE
            && mState != StatusBarState.KEYGUARD;

        if (force || mCarrierLabelVisible != makeVisible) {
            mCarrierLabelVisible = makeVisible;
            if (DEBUG) {
                Log.d(TAG, "making carrier label " + (makeVisible?"visible":"invisible"));
            }
            mCarrierLabel.animate().cancel();
            if (makeVisible) {
                mCarrierLabel.setVisibility(View.VISIBLE);
            }
            mCarrierLabel.animate()
                .alpha(makeVisible ? 1f : 0f)
                //.setStartDelay(makeVisible ? 500 : 0)
                //.setDuration(makeVisible ? 750 : 100)
                .setDuration(150)
                .setListener(makeVisible ? null : new AnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(Animator animation) {
                        if (!mCarrierLabelVisible) { // race
                            mCarrierLabel.setVisibility(View.INVISIBLE);
                            mCarrierLabel.setAlpha(0f);
                        }
                    }
                })
                .start();
        }
    }

    @Override
    protected void setAreThereNotifications() {

        if (SPEW) {
            final boolean clearable = hasActiveNotifications() &&
                    mNotificationData.hasActiveClearableNotifications();
            Log.d(TAG, "setAreThereNotifications: N=" +
                    mNotificationData.getActiveNotifications().size() + " any=" +
                    hasActiveNotifications() + " clearable=" + clearable);
        }

        final View nlo = mStatusBarView.findViewById(R.id.notification_lights_out);
        final boolean showDot = hasActiveNotifications() && !areLightsOn();
        if (showDot != (nlo.getAlpha() == 1.0f)) {
            if (showDot) {
                nlo.setAlpha(0f);
                nlo.setVisibility(View.VISIBLE);
            }
            nlo.animate()
                .alpha(showDot?1:0)
                .setDuration(showDot?750:250)
                .setInterpolator(new AccelerateInterpolator(2.0f))
                .setListener(showDot ? null : new AnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(Animator _a) {
                        nlo.setVisibility(View.GONE);
                    }
                })
                .start();
        }

        findAndUpdateMediaNotifications();

        updateCarrierLabelVisibility(false);
    }

    public void findAndUpdateMediaNotifications() {
        boolean metaDataChanged = false;

        synchronized (mNotificationData) {
            ArrayList<Entry> activeNotifications = mNotificationData.getActiveNotifications();
            final int N = activeNotifications.size();
            Entry mediaNotification = null;
            MediaController controller = null;
            for (int i = 0; i < N; i++) {
                final Entry entry = activeNotifications.get(i);
                if (isMediaNotification(entry)) {
                    final MediaSession.Token token = entry.notification.getNotification().extras
                            .getParcelable(Notification.EXTRA_MEDIA_SESSION);
                    if (token != null) {
                        controller = new MediaController(mContext, token);
                        if (controller != null) {
                            // we've got a live one, here
                            mediaNotification = entry;
                        }
                    }
                }
            }

            if (mediaNotification == null) {
                // Still nothing? OK, let's just look for live media sessions and see if they match
                // one of our notifications. This will catch apps that aren't (yet!) using media
                // notifications.

                if (mMediaSessionManager != null) {
                    final List<MediaController> sessions
                            = mMediaSessionManager.getActiveSessionsForUser(
                                    null,
                                    UserHandle.USER_ALL);

                    for (MediaController aController : sessions) {
                        if (aController == null) continue;
                        final PlaybackState state = aController.getPlaybackState();
                        if (state == null) continue;
                        switch (state.getState()) {
                            case PlaybackState.STATE_STOPPED:
                            case PlaybackState.STATE_ERROR:
                                continue;
                            default:
                                // now to see if we have one like this
                                final String pkg = aController.getPackageName();

                                for (int i = 0; i < N; i++) {
                                    final Entry entry = activeNotifications.get(i);
                                    if (entry.notification.getPackageName().equals(pkg)) {
                                        if (DEBUG_MEDIA) {
                                            Log.v(TAG, "DEBUG_MEDIA: found controller matching "
                                                + entry.notification.getKey());
                                        }
                                        controller = aController;
                                        mediaNotification = entry;
                                        break;
                                    }
                                }
                        }
                    }
                }
            }

            if (!sameSessions(mMediaController, controller)) {
                // We have a new media session

                if (mMediaController != null) {
                    // something old was playing
                    Log.v(TAG, "DEBUG_MEDIA: Disconnecting from old controller: "
                            + mMediaController);
                    mMediaController.unregisterCallback(mMediaListener);
                }
                mMediaController = controller;

                if (mMediaController != null) {
                    mMediaController.registerCallback(mMediaListener);
                    mMediaMetadata = mMediaController.getMetadata();
                    if (DEBUG_MEDIA) {
                        Log.v(TAG, "DEBUG_MEDIA: insert listener, receive metadata: "
                                + mMediaMetadata);
                    }

                    final String notificationKey = mediaNotification == null
                            ? null
                            : mediaNotification.notification.getKey();

                    if (notificationKey == null || !notificationKey.equals(mMediaNotificationKey)) {
                        // we have a new notification!
                        if (DEBUG_MEDIA) {
                            Log.v(TAG, "DEBUG_MEDIA: Found new media notification: key="
                                    + notificationKey + " controller=" + controller);
                        }
                        mMediaNotificationKey = notificationKey;
                    }
                } else {
                    mMediaMetadata = null;
                    mMediaNotificationKey = null;
                }

                metaDataChanged = true;
            } else {
                // Media session unchanged

                if (DEBUG_MEDIA) {
                    Log.v(TAG, "DEBUG_MEDIA: Continuing media notification: key=" + mMediaNotificationKey);
                }
            }
        }

        updateMediaMetaData(metaDataChanged);
    }

    private boolean sameSessions(MediaController a, MediaController b) {
        if (a == b) return true;
        if (a == null) return false;
        return a.controlsSameSession(b);
    }

    /**
     * Hide the album artwork that is fading out and release its bitmap.
     */
    private Runnable mHideBackdropFront = new Runnable() {
        @Override
        public void run() {
            if (DEBUG_MEDIA) {
                Log.v(TAG, "DEBUG_MEDIA: removing fade layer");
            }
            mBackdropFront.setVisibility(View.INVISIBLE);
            mBackdropFront.animate().cancel();
            mBackdropFront.setImageDrawable(null);
        }
    };

    /**
     * Refresh or remove lockscreen artwork from media metadata.
     */
    public void updateMediaMetaData(boolean metaDataChanged) {
        if (!SHOW_LOCKSCREEN_MEDIA_ARTWORK) return;

        if (mBackdrop == null) return; // called too early

        if (DEBUG_MEDIA) {
            Log.v(TAG, "DEBUG_MEDIA: updating album art for notification " + mMediaNotificationKey
                + " metadata=" + mMediaMetadata
                + " metaDataChanged=" + metaDataChanged
                + " state=" + mState);
        }

        Bitmap artworkBitmap = null;
        if (mMediaMetadata != null) {
            artworkBitmap = mMediaMetadata.getBitmap(MediaMetadata.METADATA_KEY_ART);
            if (artworkBitmap == null) {
                artworkBitmap = mMediaMetadata.getBitmap(MediaMetadata.METADATA_KEY_ALBUM_ART);
                // might still be null
            }
        }

        final boolean hasArtwork = artworkBitmap != null;

        if ((hasArtwork || DEBUG_MEDIA_FAKE_ARTWORK)
                && (mState == StatusBarState.KEYGUARD || mState == StatusBarState.SHADE_LOCKED)) {
            // time to show some art!
            if (mBackdrop.getVisibility() != View.VISIBLE) {
                mBackdrop.setVisibility(View.VISIBLE);
                mBackdrop.animate().alpha(1f);
                metaDataChanged = true;
                if (DEBUG_MEDIA) {
                    Log.v(TAG, "DEBUG_MEDIA: Fading in album artwork");
                }
            }
            if (metaDataChanged) {
                if (mBackdropBack.getDrawable() != null) {
                    Drawable drawable = mBackdropBack.getDrawable();
                    mBackdropFront.setImageDrawable(drawable);
                    if (mScrimSrcModeEnabled) {
                        mBackdropFront.getDrawable().mutate().setXfermode(mSrcOverXferMode);
                    }
                    mBackdropFront.setAlpha(1f);
                    mBackdropFront.setVisibility(View.VISIBLE);
                } else {
                    mBackdropFront.setVisibility(View.INVISIBLE);
                }

                if (DEBUG_MEDIA_FAKE_ARTWORK) {
                    final int c = 0xFF000000 | (int)(Math.random() * 0xFFFFFF);
                    Log.v(TAG, String.format("DEBUG_MEDIA: setting new color: 0x%08x", c));
                    mBackdropBack.setBackgroundColor(0xFFFFFFFF);
                    mBackdropBack.setImageDrawable(new ColorDrawable(c));
                } else {
                    mBackdropBack.setImageBitmap(artworkBitmap);
                }
                if (mScrimSrcModeEnabled) {
                    mBackdropBack.getDrawable().mutate().setXfermode(mSrcXferMode);
                }

                if (mBackdropFront.getVisibility() == View.VISIBLE) {
                    if (DEBUG_MEDIA) {
                        Log.v(TAG, "DEBUG_MEDIA: Crossfading album artwork from "
                                + mBackdropFront.getDrawable()
                                + " to "
                                + mBackdropBack.getDrawable());
                    }
                    mBackdropFront.animate()
                            .setDuration(250)
                            .alpha(0f).withEndAction(mHideBackdropFront);
                }
            }
        } else {
            // need to hide the album art, either because we are unlocked or because
            // the metadata isn't there to support it
            if (mBackdrop.getVisibility() != View.GONE) {
                if (DEBUG_MEDIA) {
                    Log.v(TAG, "DEBUG_MEDIA: Fading out album artwork");
                }
                mBackdrop.animate()
                        .alpha(0f)
                        .setInterpolator(mBackdropInterpolator)
                        .setDuration(300)
                        .setStartDelay(0)
                        .withEndAction(new Runnable() {
                            @Override
                            public void run() {
                                mBackdrop.setVisibility(View.GONE);
                                mBackdropFront.animate().cancel();
                                mBackdropBack.animate().cancel();
                                mHandler.post(mHideBackdropFront);
                            }
                        });
                if (mKeyguardFadingAway) {
                    mBackdrop.animate()

                            // Make it disappear faster, as the focus should be on the activity behind.
                            .setDuration(mKeyguardFadingAwayDuration / 2)
                            .setStartDelay(mKeyguardFadingAwayDelay)
                            .setInterpolator(mLinearInterpolator)
                            .start();
                }
            }
        }
    }

    public void showClock(boolean show) {
        if (mStatusBarView == null) return;
        View clock = mStatusBarView.findViewById(R.id.clock);
        if (clock != null) {
            clock.setVisibility(show ? View.VISIBLE : View.GONE);
        }
    }

    private int adjustDisableFlags(int state) {
        if (!mLaunchTransitionFadingAway
                && (mExpandedVisible || mBouncerShowing || mWaitingForKeyguardExit)) {
            state |= StatusBarManager.DISABLE_NOTIFICATION_ICONS;
            state |= StatusBarManager.DISABLE_SYSTEM_INFO;
        }
        return state;
    }

    /**
     * State is one or more of the DISABLE constants from StatusBarManager.
     */
    public void disable(int state, boolean animate) {
        mDisabledUnmodified = state;
        state = adjustDisableFlags(state);
        final int old = mDisabled;
        final int diff = state ^ old;
        mDisabled = state;
        if (DEBUG) {
            Log.d(TAG, String.format("disable: 0x%08x -> 0x%08x (diff: 0x%08x)",
                old, state, diff));
        }

        StringBuilder flagdbg = new StringBuilder();
        flagdbg.append("disable: < ");
        flagdbg.append(((state & StatusBarManager.DISABLE_EXPAND) != 0) ? "EXPAND" : "expand");
        flagdbg.append(((diff  & StatusBarManager.DISABLE_EXPAND) != 0) ? "* " : " ");
        flagdbg.append(((state & StatusBarManager.DISABLE_NOTIFICATION_ICONS) != 0) ? "ICONS" : "icons");
        flagdbg.append(((diff  & StatusBarManager.DISABLE_NOTIFICATION_ICONS) != 0) ? "* " : " ");
        flagdbg.append(((state & StatusBarManager.DISABLE_NOTIFICATION_ALERTS) != 0) ? "ALERTS" : "alerts");
        flagdbg.append(((diff  & StatusBarManager.DISABLE_NOTIFICATION_ALERTS) != 0) ? "* " : " ");
        flagdbg.append(((state & StatusBarManager.DISABLE_SYSTEM_INFO) != 0) ? "SYSTEM_INFO" : "system_info");
        flagdbg.append(((diff  & StatusBarManager.DISABLE_SYSTEM_INFO) != 0) ? "* " : " ");
        flagdbg.append(((state & StatusBarManager.DISABLE_BACK) != 0) ? "BACK" : "back");
        flagdbg.append(((diff  & StatusBarManager.DISABLE_BACK) != 0) ? "* " : " ");
        flagdbg.append(((state & StatusBarManager.DISABLE_HOME) != 0) ? "HOME" : "home");
        flagdbg.append(((diff  & StatusBarManager.DISABLE_HOME) != 0) ? "* " : " ");
        flagdbg.append(((state & StatusBarManager.DISABLE_RECENT) != 0) ? "RECENT" : "recent");
        flagdbg.append(((diff  & StatusBarManager.DISABLE_RECENT) != 0) ? "* " : " ");
        flagdbg.append(((state & StatusBarManager.DISABLE_CLOCK) != 0) ? "CLOCK" : "clock");
        flagdbg.append(((diff  & StatusBarManager.DISABLE_CLOCK) != 0) ? "* " : " ");
        flagdbg.append(((state & StatusBarManager.DISABLE_SEARCH) != 0) ? "SEARCH" : "search");
        flagdbg.append(((diff  & StatusBarManager.DISABLE_SEARCH) != 0) ? "* " : " ");
        flagdbg.append(">");
        Log.d(TAG, flagdbg.toString());

        if ((diff & StatusBarManager.DISABLE_SYSTEM_INFO) != 0) {
            mSystemIconArea.animate().cancel();
            if ((state & StatusBarManager.DISABLE_SYSTEM_INFO) != 0) {
                animateStatusBarHide(mSystemIconArea, animate);
            } else {
                animateStatusBarShow(mSystemIconArea, animate);
            }
        }

        if ((diff & StatusBarManager.DISABLE_CLOCK) != 0) {
            boolean show = (state & StatusBarManager.DISABLE_CLOCK) == 0;
            showClock(show);
        }
        if ((diff & StatusBarManager.DISABLE_EXPAND) != 0) {
            if ((state & StatusBarManager.DISABLE_EXPAND) != 0) {
                animateCollapsePanels();
            }
        }

        if ((diff & (StatusBarManager.DISABLE_HOME
                        | StatusBarManager.DISABLE_RECENT
                        | StatusBarManager.DISABLE_BACK
                        | StatusBarManager.DISABLE_SEARCH)) != 0) {
            // the nav bar will take care of these
            if (mNavigationBarView != null){
                mNavigationBarView.setDisabledFlags(state);
            }
            if ((state & StatusBarManager.DISABLE_RECENT) != 0) {
                // close recents if it's visible
                mHandler.removeMessages(MSG_HIDE_RECENT_APPS);
                mHandler.sendEmptyMessage(MSG_HIDE_RECENT_APPS);
            }
        }

        if ((diff & StatusBarManager.DISABLE_NOTIFICATION_ICONS) != 0) {
            if ((state & StatusBarManager.DISABLE_NOTIFICATION_ICONS) != 0) {
                if (mTicking) {
                    haltTicker();
                }
                animateStatusBarHide(mNotificationIconArea, animate);
             } else{
                animateStatusBarShow(mNotificationIconArea, animate);
            }
        }

        if ((diff & StatusBarManager.DISABLE_NOTIFICATION_ALERTS) != 0) {
            mDisableNotificationAlerts =
                    (state & StatusBarManager.DISABLE_NOTIFICATION_ALERTS) != 0;
            mHeadsUpObserver.onChange(true);
        }
    }

    /**
     * Animates {@code v}, a view that is part of the status bar, out.
     */
    private void animateStatusBarHide(final View v, boolean animate) {
        v.animate().cancel();
        if (!animate) {
            v.setAlpha(0f);
            v.setVisibility(View.INVISIBLE);
            return;
        }
        v.animate()
                .alpha(0f)
                .setDuration(160)
                .setStartDelay(0)
                .setInterpolator(ALPHA_OUT)
                .withEndAction(new Runnable() {
                    @Override
                    public void run() {
                        v.setVisibility(View.INVISIBLE);
                    }
                });
    }

    /**
     * Animates {@code v}, a view that is part of the status bar, in.
     */
    private void animateStatusBarShow(View v, boolean animate) {
        v.animate().cancel();
        v.setVisibility(View.VISIBLE);
        if (!animate) {
            v.setAlpha(1f);
            return;
        }
        v.animate()
                .alpha(1f)
                .setDuration(320)
                .setInterpolator(ALPHA_IN)
                .setStartDelay(50)

                // We need to clean up any pending end action from animateStatusBarHide if we call
                // both hide and show in the same frame before the animation actually gets started.
                // cancel() doesn't really remove the end action.
                .withEndAction(null);

        // Synchronize the motion with the Keyguard fading if necessary.
        if (mKeyguardFadingAway) {
            v.animate()
                    .setDuration(mKeyguardFadingAwayDuration)
                    .setInterpolator(mLinearOutSlowIn)
                    .setStartDelay(mKeyguardFadingAwayDelay)
                    .start();
        }
    }

    @Override
    protected BaseStatusBar.H createHandler() {
        return new PhoneStatusBar.H();
    }

    @Override
    public void startActivity(Intent intent, boolean dismissShade) {
        startActivityDismissingKeyguard(intent, false, dismissShade);
    }

    public ScrimController getScrimController() {
        return mScrimController;
    }

    public void setQsExpanded(boolean expanded) {
        mStatusBarWindowManager.setQsExpanded(expanded);
    }

    public boolean isGoingToNotificationShade() {
        return mLeaveOpenOnKeyguardHide;
    }

    public boolean isQsExpanded() {
        return mNotificationPanel.isQsExpanded();
    }

    public boolean isScreenOnComingFromTouch() {
        return mScreenOnComingFromTouch;
    }

    public boolean isFalsingThresholdNeeded() {
        boolean onKeyguard = getBarState() == StatusBarState.KEYGUARD;
        boolean isCurrentlyInsecure = mUnlockMethodCache.isCurrentlyInsecure();
        return onKeyguard && (isCurrentlyInsecure || mDozing || mScreenOnComingFromTouch);
    }

    public boolean isDozing() {
        return mDozing;
    }

    @Override  // NotificationData.Environment
    public String getCurrentMediaNotificationKey() {
        return mMediaNotificationKey;
    }

    public boolean isScrimSrcModeEnabled() {
        return mScrimSrcModeEnabled;
    }

    /**
     * To be called when there's a state change in StatusBarKeyguardViewManager.
     */
    public void onKeyguardViewManagerStatesUpdated() {
        logStateToEventlog();
    }

    @Override  // UnlockMethodCache.OnUnlockMethodChangedListener
    public void onUnlockMethodStateChanged() {
        logStateToEventlog();
    }

    /**
     * All changes to the status bar and notifications funnel through here and are batched.
     */
    private class H extends BaseStatusBar.H {
        public void handleMessage(Message m) {
            super.handleMessage(m);
            switch (m.what) {
                case MSG_OPEN_NOTIFICATION_PANEL:
                    animateExpandNotificationsPanel();
                    break;
                case MSG_OPEN_SETTINGS_PANEL:
                    animateExpandSettingsPanel();
                    break;
                case MSG_CLOSE_PANELS:
                    animateCollapsePanels();
                    break;
                case MSG_SHOW_HEADS_UP:
                    setHeadsUpVisibility(true);
                    break;
                case MSG_DECAY_HEADS_UP:
                    mHeadsUpNotificationView.release();
                    setHeadsUpVisibility(false);
                    break;
                case MSG_HIDE_HEADS_UP:
                    mHeadsUpNotificationView.release();
                    setHeadsUpVisibility(false);
                    break;
                case MSG_ESCALATE_HEADS_UP:
                    escalateHeadsUp();
                    setHeadsUpVisibility(false);
                    break;
                case MSG_CHANGE_BAR_HIDE_STATUS:
                       changeBarHideStatus();
                       break;
                case MSG_LAUNCH_TRANSITION_TIMEOUT:
                    onLaunchTransitionTimeout();
                    break;
            }
        }
    }

    /**  if the interrupting notification had a fullscreen intent, fire it now.  */
    private void escalateHeadsUp() {
        if (mHeadsUpNotificationView.getEntry() != null) {
            final StatusBarNotification sbn = mHeadsUpNotificationView.getEntry().notification;
            mHeadsUpNotificationView.release();
            final Notification notification = sbn.getNotification();
            if (notification.fullScreenIntent != null) {
                if (DEBUG)
                    Log.d(TAG, "converting a heads up to fullScreen");
                try {
                    EventLog.writeEvent(EventLogTags.SYSUI_HEADS_UP_ESCALATION,
                            sbn.getKey());
                    notification.fullScreenIntent.send();
                } catch (PendingIntent.CanceledException e) {
                }
            }
        }
    }

    View.OnFocusChangeListener mFocusChangeListener = new View.OnFocusChangeListener() {
        public void onFocusChange(View v, boolean hasFocus) {
            // Because 'v' is a ViewGroup, all its children will be (un)selected
            // too, which allows marqueeing to work.
            v.setSelected(hasFocus);
        }
    };

    boolean panelsEnabled() {
        return (mDisabled & StatusBarManager.DISABLE_EXPAND) == 0;
    }

    void makeExpandedVisible(boolean force) {
        if (SPEW) Log.d(TAG, "Make expanded visible: expanded visible=" + mExpandedVisible);
        if (!force && (mExpandedVisible || !panelsEnabled())) {
            return;
        }

        mExpandedVisible = true;
        //$_RBOX_$_modify_huangjc add hide button
        if (mNavigationBarView != null && mBarIsAdd)
            mNavigationBarView.setSlippery(true);

        updateCarrierLabelVisibility(true);

        updateExpandedViewPos(EXPANDED_LEAVE_ALONE);

        // Expand the window to encompass the full screen in anticipation of the drag.
        // This is only possible to do atomically because the status bar is at the top of the screen!
        mStatusBarWindowManager.setStatusBarExpanded(true);
        mStatusBarView.setFocusable(false);
        
        visibilityChanged(true);
        mWaitingForKeyguardExit = false;
        disable(mDisabledUnmodified, !force /* animate */);
        setInteracting(StatusBarManager.WINDOW_STATUS_BAR, true);
    }

    public void animateCollapsePanels() {
        animateCollapsePanels(CommandQueue.FLAG_EXCLUDE_NONE);
    }

    private final Runnable mAnimateCollapsePanels = new Runnable() {
        @Override
        public void run() {
            animateCollapsePanels();
        }
    };

    public void postAnimateCollapsePanels() {
        mHandler.post(mAnimateCollapsePanels);
    }

    public void animateCollapsePanels(int flags) {
        animateCollapsePanels(flags, false /* force */);
    }

    public void animateCollapsePanels(int flags, boolean force) {
        if (!force &&
                (mState == StatusBarState.KEYGUARD || mState == StatusBarState.SHADE_LOCKED)) {
            runPostCollapseRunnables();
            return;
        }
        if (SPEW) {
            Log.d(TAG, "animateCollapse():"
                    + " mExpandedVisible=" + mExpandedVisible
                    + " flags=" + flags);
        }

        if ((flags & CommandQueue.FLAG_EXCLUDE_RECENTS_PANEL) == 0) {
            if (!mHandler.hasMessages(MSG_HIDE_RECENT_APPS)) {
                mHandler.removeMessages(MSG_HIDE_RECENT_APPS);
                mHandler.sendEmptyMessage(MSG_HIDE_RECENT_APPS);
            }
        }

        if ((flags & CommandQueue.FLAG_EXCLUDE_SEARCH_PANEL) == 0) {
            mHandler.removeMessages(MSG_CLOSE_SEARCH_PANEL);
            mHandler.sendEmptyMessage(MSG_CLOSE_SEARCH_PANEL);
        }

        if (mStatusBarWindow != null) {
            // release focus immediately to kick off focus change transition
            mStatusBarWindowManager.setStatusBarFocusable(false);

            mStatusBarWindow.cancelExpandHelper();
            mStatusBarView.collapseAllPanels(true);
        }
    }

    private void runPostCollapseRunnables() {
        int size = mPostCollapseRunnables.size();
        for (int i = 0; i < size; i++) {
            mPostCollapseRunnables.get(i).run();
        }
        mPostCollapseRunnables.clear();
    }

    public ViewPropertyAnimator setVisibilityWhenDone(
            final ViewPropertyAnimator a, final View v, final int vis) {
        a.setListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                v.setVisibility(vis);
                a.setListener(null); // oneshot
            }
        });
        return a;
    }

    public Animator setVisibilityWhenDone(
            final Animator a, final View v, final int vis) {
        a.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                v.setVisibility(vis);
            }
        });
        return a;
    }

    public Animator interpolator(TimeInterpolator ti, Animator a) {
        a.setInterpolator(ti);
        return a;
    }

    public Animator startDelay(int d, Animator a) {
        a.setStartDelay(d);
        return a;
    }

    public Animator start(Animator a) {
        a.start();
        return a;
    }

    final TimeInterpolator mAccelerateInterpolator = new AccelerateInterpolator();
    final TimeInterpolator mDecelerateInterpolator = new DecelerateInterpolator();
    final int FLIP_DURATION_OUT = 125;
    final int FLIP_DURATION_IN = 225;
    final int FLIP_DURATION = (FLIP_DURATION_IN + FLIP_DURATION_OUT);

    Animator mScrollViewAnim, mClearButtonAnim;

    @Override
    public void animateExpandNotificationsPanel() {
        if (SPEW) Log.d(TAG, "animateExpand: mExpandedVisible=" + mExpandedVisible);
        if (!panelsEnabled()) {
            return ;
        }

        mNotificationPanel.expand();
        if (false) postStartTracing();
    }

    @Override
    public void animateExpandSettingsPanel() {
        if (SPEW) Log.d(TAG, "animateExpand: mExpandedVisible=" + mExpandedVisible);
        if (!panelsEnabled()) {
            return;
        }

        // Settings are not available in setup
        if (!mUserSetup) return;

        mNotificationPanel.expandWithQs();

        if (false) postStartTracing();
    }

    public void animateCollapseQuickSettings() {
        if (mState == StatusBarState.SHADE) {
            mStatusBarView.collapseAllPanels(true);
        }
    }

    void makeExpandedInvisible() {
        if (SPEW) Log.d(TAG, "makeExpandedInvisible: mExpandedVisible=" + mExpandedVisible
                + " mExpandedVisible=" + mExpandedVisible);

        if (!mExpandedVisible || mStatusBarWindow == null) {
            return;
        }

        // Ensure the panel is fully collapsed (just in case; bug 6765842, 7260868)
        mStatusBarView.collapseAllPanels(/*animate=*/ false);

        // reset things to their proper state
        if (mScrollViewAnim != null) mScrollViewAnim.cancel();
        if (mClearButtonAnim != null) mClearButtonAnim.cancel();

        mStackScroller.setVisibility(View.VISIBLE);
        mNotificationPanel.setVisibility(View.GONE);

        mNotificationPanel.closeQs();

        mExpandedVisible = false;
        if (mNavigationBarView != null)
            mNavigationBarView.setSlippery(false);
        visibilityChanged(false);

        // Shrink the window to the size of the status bar only
        mStatusBarWindowManager.setStatusBarExpanded(false);
        mStatusBarView.setFocusable(true);
        
        // Close any "App info" popups that might have snuck on-screen
        dismissPopups();

        runPostCollapseRunnables();
        setInteracting(StatusBarManager.WINDOW_STATUS_BAR, false);
        showBouncer();
        disable(mDisabledUnmodified, true /* animate */);

        // Trimming will happen later if Keyguard is showing - doing it here might cause a jank in
        // the bouncer appear animation.
        if (!mStatusBarKeyguardViewManager.isShowing()) {
            WindowManagerGlobal.getInstance().trimMemory(ComponentCallbacks2.TRIM_MEMORY_UI_HIDDEN);
        }
    }

    public boolean interceptTouchEvent(MotionEvent event) {
        if (DEBUG_GESTURES) {
            if (event.getActionMasked() != MotionEvent.ACTION_MOVE) {
                EventLog.writeEvent(EventLogTags.SYSUI_STATUSBAR_TOUCH,
                        event.getActionMasked(), (int) event.getX(), (int) event.getY(), mDisabled);
            }

        }

        if (SPEW) {
            Log.d(TAG, "Touch: rawY=" + event.getRawY() + " event=" + event + " mDisabled="
                + mDisabled + " mTracking=" + mTracking);
        } else if (CHATTY) {
            if (event.getAction() != MotionEvent.ACTION_MOVE) {
                Log.d(TAG, String.format(
                            "panel: %s at (%f, %f) mDisabled=0x%08x",
                            MotionEvent.actionToString(event.getAction()),
                            event.getRawX(), event.getRawY(), mDisabled));
            }
        }

        if (DEBUG_GESTURES) {
            mGestureRec.add(event);
        }

        if (mStatusBarWindowState == WINDOW_STATE_SHOWING) {
            final boolean upOrCancel =
                    event.getAction() == MotionEvent.ACTION_UP ||
                    event.getAction() == MotionEvent.ACTION_CANCEL;
            if (upOrCancel && !mExpandedVisible) {
                setInteracting(StatusBarManager.WINDOW_STATUS_BAR, false);
            } else {
                setInteracting(StatusBarManager.WINDOW_STATUS_BAR, true);
            }
        }
        return false;
    }

    public GestureRecorder getGestureRecorder() {
        return mGestureRec;
    }

    private void setNavigationIconHints(int hints) {
        if (hints == mNavigationIconHints) return;

        mNavigationIconHints = hints;

        if (mNavigationBarView != null) {
            mNavigationBarView.setNavigationIconHints(hints);
        }
        checkBarModes();
    }

    @Override // CommandQueue
    public void setWindowState(int window, int state) {
        boolean showing = state == WINDOW_STATE_SHOWING;
        if (mStatusBarWindow != null
                && window == StatusBarManager.WINDOW_STATUS_BAR
                && mStatusBarWindowState != state) {
            mStatusBarWindowState = state;
            if (DEBUG_WINDOW_STATE) Log.d(TAG, "Status bar " + windowStateToString(state));
            if (!showing && mState == StatusBarState.SHADE) {
                mStatusBarView.collapseAllPanels(false);
            }
        }
        if (mNavigationBarView != null
                && window == StatusBarManager.WINDOW_NAVIGATION_BAR
                && mNavigationBarWindowState != state) {
            mNavigationBarWindowState = state;
            if (DEBUG_WINDOW_STATE) Log.d(TAG, "Navigation bar " + windowStateToString(state));
        }
    }

    @Override // CommandQueue
    public void buzzBeepBlinked() {
        if (mDozeServiceHost != null) {
            mDozeServiceHost.fireBuzzBeepBlinked();
        }
    }

    @Override
    public void notificationLightOff() {
        if (mDozeServiceHost != null) {
            mDozeServiceHost.fireNotificationLight(false);
        }
    }

    @Override
    public void notificationLightPulse(int argb, int onMillis, int offMillis) {
        if (mDozeServiceHost != null) {
            mDozeServiceHost.fireNotificationLight(true);
        }
    }

    @Override // CommandQueue
    public void setSystemUiVisibility(int vis, int mask) {
        final int oldVal = mSystemUiVisibility;
        final int newVal = (oldVal&~mask) | (vis&mask);
        final int diff = newVal ^ oldVal;
        if (DEBUG) Log.d(TAG, String.format(
                "setSystemUiVisibility vis=%s mask=%s oldVal=%s newVal=%s diff=%s",
                Integer.toHexString(vis), Integer.toHexString(mask),
                Integer.toHexString(oldVal), Integer.toHexString(newVal),
                Integer.toHexString(diff)));
        if (diff != 0) {
            // we never set the recents bit via this method, so save the prior state to prevent
            // clobbering the bit below
            final boolean wasRecentsVisible = (mSystemUiVisibility & View.RECENT_APPS_VISIBLE) > 0;

            mSystemUiVisibility = newVal;

            // update low profile
            if ((diff & View.SYSTEM_UI_FLAG_LOW_PROFILE) != 0) {
                final boolean lightsOut = (vis & View.SYSTEM_UI_FLAG_LOW_PROFILE) != 0;
                if (lightsOut) {
                    animateCollapsePanels();
                    if (mTicking) {
                        haltTicker();
                    }
                }

                setAreThereNotifications();
            }

            // update status bar mode
            final int sbMode = computeBarMode(oldVal, newVal, mStatusBarView.getBarTransitions(),
                    View.STATUS_BAR_TRANSIENT, View.STATUS_BAR_TRANSLUCENT);

            // update navigation bar mode
            final int nbMode = mNavigationBarView == null ? -1 : computeBarMode(
                    oldVal, newVal, mNavigationBarView.getBarTransitions(),
                    View.NAVIGATION_BAR_TRANSIENT, View.NAVIGATION_BAR_TRANSLUCENT);
            final boolean sbModeChanged = sbMode != -1;
            final boolean nbModeChanged = nbMode != -1;
            boolean checkBarModes = false;
            if (sbModeChanged && sbMode != mStatusBarMode) {
                mStatusBarMode = sbMode;
                checkBarModes = true;
            }
            if (nbModeChanged && nbMode != mNavigationBarMode) {
                mNavigationBarMode = nbMode;
                checkBarModes = true;
            }
            if (checkBarModes) {
                checkBarModes();
            }
            if (sbModeChanged || nbModeChanged) {
                // update transient bar autohide
                if (mStatusBarMode == MODE_SEMI_TRANSPARENT || mNavigationBarMode == MODE_SEMI_TRANSPARENT) {
                    scheduleAutohide();
                } else {
                    cancelAutohide();
                }
            }

            // ready to unhide
            if ((vis & View.STATUS_BAR_UNHIDE) != 0) {
                mSystemUiVisibility &= ~View.STATUS_BAR_UNHIDE;
            }
            if ((vis & View.NAVIGATION_BAR_UNHIDE) != 0) {
                mSystemUiVisibility &= ~View.NAVIGATION_BAR_UNHIDE;
            }

            // restore the recents bit
            if (wasRecentsVisible) {
                mSystemUiVisibility |= View.RECENT_APPS_VISIBLE;
            }

            // send updated sysui visibility to window manager
            notifyUiVisibilityChanged(mSystemUiVisibility);
        }
    }

    private int computeBarMode(int oldVis, int newVis, BarTransitions transitions,
            int transientFlag, int translucentFlag) {
        final int oldMode = barMode(oldVis, transientFlag, translucentFlag);
        final int newMode = barMode(newVis, transientFlag, translucentFlag);
        if (oldMode == newMode) {
            return -1; // no mode change
        }
        return newMode;
    }

    private int barMode(int vis, int transientFlag, int translucentFlag) {
        int lightsOutTransparent = View.SYSTEM_UI_FLAG_LOW_PROFILE | View.SYSTEM_UI_TRANSPARENT;
        return (vis & transientFlag) != 0 ? MODE_SEMI_TRANSPARENT
                : (vis & translucentFlag) != 0 ? MODE_TRANSLUCENT
                : (vis & lightsOutTransparent) == lightsOutTransparent ? MODE_LIGHTS_OUT_TRANSPARENT
                : (vis & View.SYSTEM_UI_TRANSPARENT) != 0 ? MODE_TRANSPARENT
                : (vis & View.SYSTEM_UI_FLAG_LOW_PROFILE) != 0 ? MODE_LIGHTS_OUT
                : MODE_OPAQUE;
    }

    private void checkBarModes() {
        if (mDemoMode) return;
        checkBarMode(mStatusBarMode, mStatusBarWindowState, mStatusBarView.getBarTransitions());
        if (mNavigationBarView != null) {
            checkBarMode(mNavigationBarMode,
                    mNavigationBarWindowState, mNavigationBarView.getBarTransitions());
        }
    }

    private void checkBarMode(int mode, int windowState, BarTransitions transitions) {
        final boolean powerSave = mBatteryController.isPowerSave();
        final boolean anim = (mScreenOn == null || mScreenOn) && windowState != WINDOW_STATE_HIDDEN
                && !powerSave;
        if (powerSave && getBarState() == StatusBarState.SHADE) {
            mode = MODE_WARNING;
        }
        transitions.transitionTo(mode, anim);
    }

    private void finishBarAnimations() {
        mStatusBarView.getBarTransitions().finishAnimations();
        if (mNavigationBarView != null) {
            mNavigationBarView.getBarTransitions().finishAnimations();
        }
    }

    private final Runnable mCheckBarModes = new Runnable() {
        @Override
        public void run() {
            checkBarModes();
        }
    };

    @Override
    public void setInteracting(int barWindow, boolean interacting) {
        final boolean changing = ((mInteractingWindows & barWindow) != 0) != interacting;
        mInteractingWindows = interacting
                ? (mInteractingWindows | barWindow)
                : (mInteractingWindows & ~barWindow);
        if (mInteractingWindows != 0) {
            suspendAutohide();
        } else {
            resumeSuspendedAutohide();
        }
        // manually dismiss the volume panel when interacting with the nav bar
        if (changing && interacting && barWindow == StatusBarManager.WINDOW_NAVIGATION_BAR) {
            if (mVolumeComponent != null) {
                mVolumeComponent.dismissNow();
            }
        }
        checkBarModes();
    }

    private void resumeSuspendedAutohide() {
        if (mAutohideSuspended) {
            scheduleAutohide();
            mHandler.postDelayed(mCheckBarModes, 500); // longer than home -> launcher
        }
    }

    private void suspendAutohide() {
        mHandler.removeCallbacks(mAutohide);
        mHandler.removeCallbacks(mCheckBarModes);
        mAutohideSuspended = (mSystemUiVisibility & STATUS_OR_NAV_TRANSIENT) != 0;
    }

    private void cancelAutohide() {
        mAutohideSuspended = false;
        mHandler.removeCallbacks(mAutohide);
    }

    private void scheduleAutohide() {
        cancelAutohide();
        mHandler.postDelayed(mAutohide, AUTOHIDE_TIMEOUT_MS);
    }

    private void checkUserAutohide(View v, MotionEvent event) {
        if ((mSystemUiVisibility & STATUS_OR_NAV_TRANSIENT) != 0  // a transient bar is revealed
                && event.getAction() == MotionEvent.ACTION_OUTSIDE // touch outside the source bar
                && event.getX() == 0 && event.getY() == 0  // a touch outside both bars
                ) {
            userAutohide();
        }
    }

    private void userAutohide() {
        cancelAutohide();
        mHandler.postDelayed(mAutohide, 350); // longer than app gesture -> flag clear
    }

    private boolean areLightsOn() {
        return 0 == (mSystemUiVisibility & View.SYSTEM_UI_FLAG_LOW_PROFILE);
    }

    public void setLightsOn(boolean on) {
        Log.v(TAG, "setLightsOn(" + on + ")");
        if (on) {
            setSystemUiVisibility(0, View.SYSTEM_UI_FLAG_LOW_PROFILE);
        } else {
            setSystemUiVisibility(View.SYSTEM_UI_FLAG_LOW_PROFILE, View.SYSTEM_UI_FLAG_LOW_PROFILE);
        }
    }

    private void notifyUiVisibilityChanged(int vis) {
        try {
            mWindowManagerService.statusBarVisibilityChanged(vis);
        } catch (RemoteException ex) {
        }
    }

    public void topAppWindowChanged(boolean showMenu) {
        if (DEBUG) {
            Log.d(TAG, (showMenu?"showing":"hiding") + " the MENU button");
        }
        if (mNavigationBarView != null) {
            mNavigationBarView.setMenuVisibility(showMenu);
        }

        // See above re: lights-out policy for legacy apps.
        if (showMenu) setLightsOn(true);
    }

    @Override
    public void setImeWindowStatus(IBinder token, int vis, int backDisposition,
            boolean showImeSwitcher) {
        boolean imeShown = (vis & InputMethodService.IME_VISIBLE) != 0;
        int flags = mNavigationIconHints;
        if ((backDisposition == InputMethodService.BACK_DISPOSITION_WILL_DISMISS) || imeShown) {
            flags |= NAVIGATION_HINT_BACK_ALT;
        } else {
            flags &= ~NAVIGATION_HINT_BACK_ALT;
        }
        if (showImeSwitcher) {
            flags |= NAVIGATION_HINT_IME_SHOWN;
        } else {
            flags &= ~NAVIGATION_HINT_IME_SHOWN;
        }

        setNavigationIconHints(flags);
    }

    @Override
    protected void tick(StatusBarNotification n, boolean firstTime) {
        if (!mTickerEnabled) return;

        // no ticking in lights-out mode
        if (!areLightsOn()) return;

        // no ticking in Setup
        if (!isDeviceProvisioned()) return;

        // not for you
        if (!isNotificationForCurrentProfiles(n)) return;

        // Show the ticker if one is requested. Also don't do this
        // until status bar window is attached to the window manager,
        // because...  well, what's the point otherwise?  And trying to
        // run a ticker without being attached will crash!
        if (n.getNotification().tickerText != null && mStatusBarWindow != null
                && mStatusBarWindow.getWindowToken() != null) {
            if (0 == (mDisabled & (StatusBarManager.DISABLE_NOTIFICATION_ICONS
                    | StatusBarManager.DISABLE_NOTIFICATION_TICKER))) {
                mTicker.addEntry(n);
            }
        }
    }

    private class MyTicker extends Ticker {
        MyTicker(Context context, View sb) {
            super(context, sb);
            if (!mTickerEnabled) {
                Log.w(TAG, "MyTicker instantiated with mTickerEnabled=false", new Throwable());
            }
        }

        @Override
        public void tickerStarting() {
            if (!mTickerEnabled) return;
            mTicking = true;
            // make it has chance to layout status bar during many tickers
            mStatusBarContents.setVisibility(View.INVISIBLE);
            mTickerView.setVisibility(View.VISIBLE);
            mTickerView.startAnimation(loadAnim(com.android.internal.R.anim.push_up_in, null));
            mStatusBarContents.startAnimation(loadAnim(com.android.internal.R.anim.push_up_out, null));
        }

        @Override
        public void tickerDone() {
            if (!mTickerEnabled) return;
            mStatusBarContents.setVisibility(View.VISIBLE);
            mTickerView.setVisibility(View.GONE);
            mStatusBarContents.startAnimation(loadAnim(com.android.internal.R.anim.push_down_in, null));
            mTickerView.startAnimation(loadAnim(com.android.internal.R.anim.push_down_out,
                        mTickingDoneListener));
        }

        public void tickerHalting() {
            if (!mTickerEnabled) return;
            if (mStatusBarContents.getVisibility() != View.VISIBLE) {
                mStatusBarContents.setVisibility(View.VISIBLE);
                mStatusBarContents
                        .startAnimation(loadAnim(com.android.internal.R.anim.fade_in, null));
            }
            mTickerView.setVisibility(View.GONE);
            // we do not animate the ticker away at this point, just get rid of it (b/6992707)
        }
    }

    Animation.AnimationListener mTickingDoneListener = new Animation.AnimationListener() {;
        public void onAnimationEnd(Animation animation) {
            mTicking = false;
        }
        public void onAnimationRepeat(Animation animation) {
        }
        public void onAnimationStart(Animation animation) {
        }
    };

    private Animation loadAnim(int id, Animation.AnimationListener listener) {
        Animation anim = AnimationUtils.loadAnimation(mContext, id);
        if (listener != null) {
            anim.setAnimationListener(listener);
        }
        return anim;
    }

    public static String viewInfo(View v) {
        return "[(" + v.getLeft() + "," + v.getTop() + ")(" + v.getRight() + "," + v.getBottom()
                + ") " + v.getWidth() + "x" + v.getHeight() + "]";
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        synchronized (mQueueLock) {
            pw.println("Current Status Bar state:");
            pw.println("  mExpandedVisible=" + mExpandedVisible
                    + ", mTrackingPosition=" + mTrackingPosition);
            pw.println("  mTickerEnabled=" + mTickerEnabled);
            if (mTickerEnabled) {
                pw.println("  mTicking=" + mTicking);
                pw.println("  mTickerView: " + viewInfo(mTickerView));
            }
            pw.println("  mTracking=" + mTracking);
            pw.println("  mDisplayMetrics=" + mDisplayMetrics);
            pw.println("  mStackScroller: " + viewInfo(mStackScroller));
            pw.println("  mStackScroller: " + viewInfo(mStackScroller)
                    + " scroll " + mStackScroller.getScrollX()
                    + "," + mStackScroller.getScrollY());
        }

        pw.print("  mInteractingWindows="); pw.println(mInteractingWindows);
        pw.print("  mStatusBarWindowState=");
        pw.println(windowStateToString(mStatusBarWindowState));
        pw.print("  mStatusBarMode=");
        pw.println(BarTransitions.modeToString(mStatusBarMode));
        pw.print("  mDozing="); pw.println(mDozing);
        pw.print("  mZenMode=");
        pw.println(Settings.Global.zenModeToString(mZenMode));
        pw.print("  mUseHeadsUp=");
        pw.println(mUseHeadsUp);
        pw.print("  interrupting package: ");
        pw.println(hunStateToString(mHeadsUpNotificationView.getEntry()));
        dumpBarTransitions(pw, "mStatusBarView", mStatusBarView.getBarTransitions());
        if (mNavigationBarView != null) {
            pw.print("  mNavigationBarWindowState=");
            pw.println(windowStateToString(mNavigationBarWindowState));
            pw.print("  mNavigationBarMode=");
            pw.println(BarTransitions.modeToString(mNavigationBarMode));
            dumpBarTransitions(pw, "mNavigationBarView", mNavigationBarView.getBarTransitions());
        }

        pw.print("  mNavigationBarView=");
        if (mNavigationBarView == null) {
            pw.println("null");
        } else {
            mNavigationBarView.dump(fd, pw, args);
        }

        pw.print("  mMediaSessionManager=");
        pw.println(mMediaSessionManager);
        pw.print("  mMediaNotificationKey=");
        pw.println(mMediaNotificationKey);
        pw.print("  mMediaController=");
        pw.print(mMediaController);
        if (mMediaController != null) {
            pw.print(" state=" + mMediaController.getPlaybackState());
        }
        pw.println();
        pw.print("  mMediaMetadata=");
        pw.print(mMediaMetadata);
        if (mMediaMetadata != null) {
            pw.print(" title=" + mMediaMetadata.getText(MediaMetadata.METADATA_KEY_TITLE));
        }
        pw.println();

        pw.println("  Panels: ");
        if (mNotificationPanel != null) {
            pw.println("    mNotificationPanel=" +
                mNotificationPanel + " params=" + mNotificationPanel.getLayoutParams().debug(""));
            pw.print  ("      ");
            mNotificationPanel.dump(fd, pw, args);
        }

        DozeLog.dump(pw);

        if (DUMPTRUCK) {
            synchronized (mNotificationData) {
                mNotificationData.dump(pw, "  ");
            }

            int N = mStatusIcons.getChildCount();
            pw.println("  system icons: " + N);
            for (int i=0; i<N; i++) {
                StatusBarIconView ic = (StatusBarIconView) mStatusIcons.getChildAt(i);
                pw.println("    [" + i + "] icon=" + ic);
            }

            if (false) {
                pw.println("see the logcat for a dump of the views we have created.");
                // must happen on ui thread
                mHandler.post(new Runnable() {
                        public void run() {
                            mStatusBarView.getLocationOnScreen(mAbsPos);
                            Log.d(TAG, "mStatusBarView: ----- (" + mAbsPos[0] + "," + mAbsPos[1]
                                    + ") " + mStatusBarView.getWidth() + "x"
                                    + getStatusBarHeight());
                            mStatusBarView.debug();
                        }
                    });
            }
        }

        if (DEBUG_GESTURES) {
            pw.print("  status bar gestures: ");
            mGestureRec.dump(fd, pw, args);
        }

        if (mNetworkController != null) {
            mNetworkController.dump(fd, pw, args);
        }
        if (mBluetoothController != null) {
            mBluetoothController.dump(fd, pw, args);
        }
        if (mCastController != null) {
            mCastController.dump(fd, pw, args);
        }
        if (mUserSwitcherController != null) {
            mUserSwitcherController.dump(fd, pw, args);
        }
        if (mBatteryController != null) {
            mBatteryController.dump(fd, pw, args);
        }
        if (mNextAlarmController != null) {
            mNextAlarmController.dump(fd, pw, args);
        }
        if (mSecurityController != null) {
            mSecurityController.dump(fd, pw, args);
        }
        pw.println("SharedPreferences:");
        for (Map.Entry<String, ?> entry : mContext.getSharedPreferences(mContext.getPackageName(),
                Context.MODE_PRIVATE).getAll().entrySet()) {
            pw.print("  "); pw.print(entry.getKey()); pw.print("="); pw.println(entry.getValue());
        }
    }

    private String hunStateToString(Entry entry) {
        if (entry == null) return "null";
        if (entry.notification == null) return "corrupt";
        return entry.notification.getPackageName();
    }

    private static void dumpBarTransitions(PrintWriter pw, String var, BarTransitions transitions) {
        pw.print("  "); pw.print(var); pw.print(".BarTransitions.mMode=");
        pw.println(BarTransitions.modeToString(transitions.getMode()));
    }

    @Override
    public void createAndAddWindows() {
		wm = (WindowManager) mContext.getSystemService(
				Context.WINDOW_SERVICE);
        addMultiModeWindow();
        addStatusBarWindow();
		addHalfScreenWindowController();
		if(IS_USE_WHCONTROLS){
			addFourScreenWindowController();
		}
		addCenterBtnWindow();
		if(ONE_LEVEL_MENU){
			addCircleMenuWindow();
		}
		if(IS_USE_BACK_WINDOW){
			if(mFourScreenBackWindow == null){
				mFourScreenBackWindow = new FourScreenBackWindow(mContext, wm);
			}
		}
		if(mMinWindow == null){
			mMinWindow = new MinWindow(mContext, wm);
		}
    }

    private void addStatusBarWindow() {
        makeStatusBarView();
        mStatusBarWindowManager = new StatusBarWindowManager(mContext);
        mStatusBarWindowManager.add(mStatusBarWindow, getStatusBarHeight());
    }

    static final float saturate(float a) {
        return a < 0f ? 0f : (a > 1f ? 1f : a);
    }

    @Override
    public void updateExpandedViewPos(int thingy) {
        if (SPEW) Log.v(TAG, "updateExpandedViewPos");

        // on larger devices, the notification panel is propped open a bit
        mNotificationPanel.setMinimumHeight(
                (int)(mNotificationPanelMinHeightFrac * mCurrentDisplaySize.y));

        FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams) mNotificationPanel.getLayoutParams();
        lp.gravity = mNotificationPanelGravity;
        mNotificationPanel.setLayoutParams(lp);

        updateCarrierLabelVisibility(false);
    }

    // called by makeStatusbar and also by PhoneStatusBarView
    void updateDisplaySize() {
        mDisplay.getMetrics(mDisplayMetrics);
        mDisplay.getSize(mCurrentDisplaySize);
        if (DEBUG_GESTURES) {
            mGestureRec.tag("display",
                    String.format("%dx%d", mDisplayMetrics.widthPixels, mDisplayMetrics.heightPixels));
        }
    }

    float getDisplayDensity() {
        return mDisplayMetrics.density;
    }

    public void startActivityDismissingKeyguard(final Intent intent, boolean onlyProvisioned,
            final boolean dismissShade) {
        if (onlyProvisioned && !isDeviceProvisioned()) return;

        final boolean afterKeyguardGone = PreviewInflater.wouldLaunchResolverActivity(
                mContext, intent, mCurrentUserId);
        final boolean keyguardShowing = mStatusBarKeyguardViewManager.isShowing();
        dismissKeyguardThenExecute(new OnDismissAction() {
            @Override
            public boolean onDismiss() {
                AsyncTask.execute(new Runnable() {
                    public void run() {
                        try {
                            if (keyguardShowing && !afterKeyguardGone) {
                                ActivityManagerNative.getDefault()
                                        .keyguardWaitingForActivityDrawn();
                            }
                            intent.setFlags(
                                    Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
                            mContext.startActivityAsUser(
                                    intent, new UserHandle(UserHandle.USER_CURRENT));
                            overrideActivityPendingAppTransition(
                                    keyguardShowing && !afterKeyguardGone);
                        } catch (RemoteException e) {
                        }
                    }
                });
                if (dismissShade) {
                    animateCollapsePanels(
                            CommandQueue.FLAG_EXCLUDE_RECENTS_PANEL, true /* force */);
                }
                return true;
            }
        }, afterKeyguardGone);
    }

    private BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            if (DEBUG) Log.v(TAG, "onReceive: " + intent);
            String action = intent.getAction();
            if (Intent.ACTION_CLOSE_SYSTEM_DIALOGS.equals(action)) {
                if (isCurrentProfile(getSendingUserId())) {
                    int flags = CommandQueue.FLAG_EXCLUDE_NONE;
                    String reason = intent.getStringExtra("reason");
                    if (reason != null && reason.equals(SYSTEM_DIALOG_REASON_RECENT_APPS)) {
                        flags |= CommandQueue.FLAG_EXCLUDE_RECENTS_PANEL;
                    }
                    animateCollapsePanels(flags);
                }
            }
            else if (Intent.ACTION_SCREEN_OFF.equals(action)) {
                mScreenOn = false;
                notifyNavigationBarScreenOn(false);
                notifyHeadsUpScreenOn(false);
                finishBarAnimations();
                resetUserExpandedStates();
            }
            else if (Intent.ACTION_SCREEN_ON.equals(action)) {
                mScreenOn = true;
                notifyNavigationBarScreenOn(true);
            }
            else if (ACTION_DEMO.equals(action)) {
                Bundle bundle = intent.getExtras();
                if (bundle != null) {
                    String command = bundle.getString("command", "").trim().toLowerCase();
                    if (command.length() > 0) {
                        try {
                            dispatchDemoCommand(command, bundle);
                        } catch (Throwable t) {
                            Log.w(TAG, "Error running demo command, intent=" + intent, t);
                        }
                    }
                }
            } else if ("fake_artwork".equals(action)) {
                if (DEBUG_MEDIA_FAKE_ARTWORK) {
                    updateMediaMetaData(true);
                }
            }  else if("com.tchip.changeBarHideStatus".equals(action))
            {
                       mHandler.removeMessages(MSG_CHANGE_BAR_HIDE_STATUS);
                       mHandler.sendEmptyMessageDelayed(MSG_CHANGE_BAR_HIDE_STATUS, 400);
                //changeBarHideStatus();
            }
        }
    };

    private void resetUserExpandedStates() {
        ArrayList<Entry> activeNotifications = mNotificationData.getActiveNotifications();
        final int notificationCount = activeNotifications.size();
        for (int i = 0; i < notificationCount; i++) {
            NotificationData.Entry entry = activeNotifications.get(i);
            if (entry.row != null) {
                entry.row.resetUserExpansion();
            }
        }
    }

    @Override
    protected void dismissKeyguardThenExecute(final OnDismissAction action,
            boolean afterKeyguardGone) {
        if (mStatusBarKeyguardViewManager.isShowing()) {
            mStatusBarKeyguardViewManager.dismissWithAction(action, afterKeyguardGone);
        } else {
            action.onDismiss();
        }
    }

    // SystemUIService notifies SystemBars of configuration changes, which then calls down here
 /*   @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig); // calls refreshLayout

        if (DEBUG) {
            Log.v(TAG, "configuration changed: " + mContext.getResources().getConfiguration());
        }
        updateDisplaySize(); // populates mDisplayMetrics

        updateResources();
        updateClockSize();
        repositionNavigationBar();
        updateExpandedViewPos(EXPANDED_LEAVE_ALONE);
        updateShowSearchHoldoff();
        updateRowStates();
        mScreenPinningRequest.onConfigurationChanged();
    }
*/
    @Override
    public void userSwitched(int newUserId) {
        super.userSwitched(newUserId);
        if (MULTIUSER_DEBUG) mNotificationPanelDebugText.setText("USER " + newUserId);
        animateCollapsePanels();
        updatePublicMode();
        updateNotifications();
        resetUserSetupObserver();
        setControllerUsers();
    }

    private void setControllerUsers() {
        if (mZenModeController != null) {
            mZenModeController.setUserId(mCurrentUserId);
        }
    }

    private void resetUserSetupObserver() {
        mContext.getContentResolver().unregisterContentObserver(mUserSetupObserver);
        mUserSetupObserver.onChange(false);
        mContext.getContentResolver().registerContentObserver(
                Settings.Secure.getUriFor(Settings.Secure.USER_SETUP_COMPLETE), true,
                mUserSetupObserver,
                mCurrentUserId);
    }

    private void setHeadsUpVisibility(boolean vis) {
        if (!ENABLE_HEADS_UP) return;
        if (DEBUG) Log.v(TAG, (vis ? "showing" : "hiding") + " heads up window");
        EventLog.writeEvent(EventLogTags.SYSUI_HEADS_UP_STATUS,
                vis ? mHeadsUpNotificationView.getKey() : "",
                vis ? 1 : 0);
        mHeadsUpNotificationView.setVisibility(vis ? View.VISIBLE : View.GONE);
    }

    public void onHeadsUpDismissed() {
        mHeadsUpNotificationView.dismiss();
    }

    /**
     * Reload some of our resources when the configuration changes.
     *
     * We don't reload everything when the configuration changes -- we probably
     * should, but getting that smooth is tough.  Someday we'll fix that.  In the
     * meantime, just update the things that we know change.
     */
    void updateResources() {
        // Update the quick setting tiles
        if (mQSPanel != null) {
            mQSPanel.updateResources();
        }

        loadDimens();
        mLinearOutSlowIn = AnimationUtils.loadInterpolator(
                mContext, android.R.interpolator.linear_out_slow_in);

        if (mNotificationPanel != null) {
            mNotificationPanel.updateResources();
        }
        if (mHeadsUpNotificationView != null) {
            mHeadsUpNotificationView.updateResources();
        }
        if (mBrightnessMirrorController != null) {
            mBrightnessMirrorController.updateResources();
        }
    }

    private void updateClockSize() {
        if (mStatusBarView == null) return;
        TextView clock = (TextView) mStatusBarView.findViewById(R.id.clock);
        if (clock != null) {
            FontSizeUtils.updateFontSize(clock, R.dimen.status_bar_clock_size);
        }
    }
    protected void loadDimens() {
        final Resources res = mContext.getResources();

        mNaturalBarHeight = res.getDimensionPixelSize(
                com.android.internal.R.dimen.status_bar_height);

        int newIconSize = res.getDimensionPixelSize(
            com.android.internal.R.dimen.status_bar_icon_size);
        int newIconHPadding = res.getDimensionPixelSize(
            R.dimen.status_bar_icon_padding);

        if (newIconHPadding != mIconHPadding || newIconSize != mIconSize) {
//            Log.d(TAG, "size=" + newIconSize + " padding=" + newIconHPadding);
            mIconHPadding = newIconHPadding;
            mIconSize = newIconSize;
            //reloadAllNotificationIcons(); // reload the tray
        }

        mEdgeBorder = res.getDimensionPixelSize(R.dimen.status_bar_edge_ignore);

        mNotificationPanelGravity = res.getInteger(R.integer.notification_panel_layout_gravity);
        if (mNotificationPanelGravity <= 0) {
            mNotificationPanelGravity = Gravity.START | Gravity.TOP;
        }

        mCarrierLabelHeight = res.getDimensionPixelSize(R.dimen.carrier_label_height);
        mStatusBarHeaderHeight = res.getDimensionPixelSize(R.dimen.status_bar_header_height);

        mNotificationPanelMinHeightFrac = res.getFraction(R.dimen.notification_panel_min_height_frac, 1, 1);
        if (mNotificationPanelMinHeightFrac < 0f || mNotificationPanelMinHeightFrac > 1f) {
            mNotificationPanelMinHeightFrac = 0f;
        }
		MAX_RAN_MULCON = (int)res.getDimension(R.dimen.max_range_mulcon);
		MUL_CON_PAD = (int)res.getDimension(R.dimen.mul_config_padding);
		MUL_IMA_SIZE = (int)res.getDimension(R.dimen.mul_image_size);
		CENTER_BTN_WIDTH_VALUE = CENTER_BTN_WIDTH = (int)res.getDimension(R.dimen.center_btn_width);
		CENTER_BTN_HEIGHT_VALUE = CENTER_BTN_HEIGHT = (int)res.getDimension(R.dimen.center_btn_height);
		mStatusBarHeight = (int)res.getDimensionPixelSize(com.android.internal.R.dimen.status_bar_height);
		MUL_CON_POS = MUL_CON_PAD+ (int)(MUL_IMA_SIZE/2);

        mHeadsUpNotificationDecay = res.getInteger(R.integer.heads_up_notification_decay);
        mRowMinHeight =  res.getDimensionPixelSize(R.dimen.notification_min_height);
        mRowMaxHeight =  res.getDimensionPixelSize(R.dimen.notification_max_height);

        mKeyguardMaxNotificationCount = res.getInteger(R.integer.keyguard_max_notification_count);

        if (DEBUG) Log.v(TAG, "updateResources");
    }

    // Visibility reporting

    @Override
    protected void handleVisibleToUserChanged(boolean visibleToUser) {
        if (visibleToUser) {
            super.handleVisibleToUserChanged(visibleToUser);
            startNotificationLogging();
        } else {
            stopNotificationLogging();
            super.handleVisibleToUserChanged(visibleToUser);
        }
    }

    private void stopNotificationLogging() {
        // Report all notifications as invisible and turn down the
        // reporter.
        if (!mCurrentlyVisibleNotifications.isEmpty()) {
            logNotificationVisibilityChanges(
                    Collections.<String>emptyList(), mCurrentlyVisibleNotifications);
            mCurrentlyVisibleNotifications.clear();
        }
        mHandler.removeCallbacks(mVisibilityReporter);
        mStackScroller.setChildLocationsChangedListener(null);
    }

    private void startNotificationLogging() {
        mStackScroller.setChildLocationsChangedListener(mNotificationLocationsChangedListener);
        // Some transitions like mVisibleToUser=false -> mVisibleToUser=true don't
        // cause the scroller to emit child location events. Hence generate
        // one ourselves to guarantee that we're reporting visible
        // notifications.
        // (Note that in cases where the scroller does emit events, this
        // additional event doesn't break anything.)
        mNotificationLocationsChangedListener.onChildLocationsChanged(mStackScroller);
    }

    private void logNotificationVisibilityChanges(
            Collection<String> newlyVisible, Collection<String> noLongerVisible) {
        if (newlyVisible.isEmpty() && noLongerVisible.isEmpty()) {
            return;
        }
        String[] newlyVisibleAr = newlyVisible.toArray(new String[newlyVisible.size()]);
        String[] noLongerVisibleAr = noLongerVisible.toArray(new String[noLongerVisible.size()]);
        try {
            mBarService.onNotificationVisibilityChanged(newlyVisibleAr, noLongerVisibleAr);
        } catch (RemoteException e) {
            // Ignore.
        }
    }

    // State logging

    private void logStateToEventlog() {
        boolean isShowing = mStatusBarKeyguardViewManager.isShowing();
        boolean isOccluded = mStatusBarKeyguardViewManager.isOccluded();
        boolean isBouncerShowing = mStatusBarKeyguardViewManager.isBouncerShowing();
        boolean isSecure = mUnlockMethodCache.isMethodSecure();
        boolean isCurrentlyInsecure = mUnlockMethodCache.isCurrentlyInsecure();
        int stateFingerprint = getLoggingFingerprint(mState,
                isShowing,
                isOccluded,
                isBouncerShowing,
                isSecure,
                isCurrentlyInsecure);
        if (stateFingerprint != mLastLoggedStateFingerprint) {
            EventLogTags.writeSysuiStatusBarState(mState,
                    isShowing ? 1 : 0,
                    isOccluded ? 1 : 0,
                    isBouncerShowing ? 1 : 0,
                    isSecure ? 1 : 0,
                    isCurrentlyInsecure ? 1 : 0);
            mLastLoggedStateFingerprint = stateFingerprint;
        }
    }

    /**
     * Returns a fingerprint of fields logged to eventlog
     */
    private static int getLoggingFingerprint(int statusBarState, boolean keyguardShowing,
            boolean keyguardOccluded, boolean bouncerShowing, boolean secure,
            boolean currentlyInsecure) {
        // Reserve 8 bits for statusBarState. We'll never go higher than
        // that, right? Riiiight.
        return (statusBarState & 0xFF)
                | ((keyguardShowing   ? 1 : 0) <<  8)
                | ((keyguardOccluded  ? 1 : 0) <<  9)
                | ((bouncerShowing    ? 1 : 0) << 10)
                | ((secure            ? 1 : 0) << 11)
                | ((currentlyInsecure ? 1 : 0) << 12);
    }

    //
    // tracing
    //

    void postStartTracing() {
        mHandler.postDelayed(mStartTracing, 3000);
    }

    void vibrate() {
        android.os.Vibrator vib = (android.os.Vibrator)mContext.getSystemService(
                Context.VIBRATOR_SERVICE);
        vib.vibrate(250, VIBRATION_ATTRIBUTES);
    }

    Runnable mStartTracing = new Runnable() {
        public void run() {
            vibrate();
            SystemClock.sleep(250);
            Log.d(TAG, "startTracing");
            android.os.Debug.startMethodTracing("/data/statusbar-traces/trace");
            mHandler.postDelayed(mStopTracing, 10000);
        }
    };

    Runnable mStopTracing = new Runnable() {
        public void run() {
            android.os.Debug.stopMethodTracing();
            Log.d(TAG, "stopTracing");
            vibrate();
        }
    };

    @Override
    protected void haltTicker() {
        if (mTickerEnabled) {
            mTicker.halt();
        }
    }

    @Override
    protected boolean shouldDisableNavbarGestures() {
        return !isDeviceProvisioned()
                || mExpandedVisible
                || (mDisabled & StatusBarManager.DISABLE_SEARCH) != 0;
    }

    public void postStartSettingsActivity(final Intent intent, int delay) {
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                handleStartSettingsActivity(intent, true /*onlyProvisioned*/);
            }
        }, delay);
    }

    private void handleStartSettingsActivity(Intent intent, boolean onlyProvisioned) {
        startActivityDismissingKeyguard(intent, onlyProvisioned, true /* dismissShade */);
    }

    private static class FastColorDrawable extends Drawable {
        private final int mColor;

        public FastColorDrawable(int color) {
            mColor = 0xff000000 | color;
        }

        @Override
        public void draw(Canvas canvas) {
            canvas.drawColor(mColor, PorterDuff.Mode.SRC);
        }

        @Override
        public void setAlpha(int alpha) {
        }

        @Override
        public void setColorFilter(ColorFilter cf) {
        }

        @Override
        public int getOpacity() {
            return PixelFormat.OPAQUE;
        }

        @Override
        public void setBounds(int left, int top, int right, int bottom) {
        }

        @Override
        public void setBounds(Rect bounds) {
        }
    }

    @Override
    public void destroy() {
        super.destroy();
        if (mStatusBarWindow != null) {
            mWindowManager.removeViewImmediate(mStatusBarWindow);
            mStatusBarWindow = null;
        }
        if (mNavigationBarView != null) {
            mWindowManager.removeViewImmediate(mNavigationBarView);
            mNavigationBarView = null;
        }
        if (mHandlerThread != null) {
            mHandlerThread.quitSafely();
            mHandlerThread = null;
        }
        mContext.unregisterReceiver(mBroadcastReceiver);
    }

    private boolean mDemoModeAllowed;
    private boolean mDemoMode;
    private DemoStatusIcons mDemoStatusIcons;

    @Override
    public void dispatchDemoCommand(String command, Bundle args) {
        if (!mDemoModeAllowed) {
            mDemoModeAllowed = Settings.Global.getInt(mContext.getContentResolver(),
                    "sysui_demo_allowed", 0) != 0;
        }
        if (!mDemoModeAllowed) return;
        if (command.equals(COMMAND_ENTER)) {
            mDemoMode = true;
        } else if (command.equals(COMMAND_EXIT)) {
            mDemoMode = false;
            checkBarModes();
        } else if (!mDemoMode) {
            // automatically enter demo mode on first demo command
            dispatchDemoCommand(COMMAND_ENTER, new Bundle());
        }
        boolean modeChange = command.equals(COMMAND_ENTER) || command.equals(COMMAND_EXIT);
        if ((modeChange || command.equals(COMMAND_VOLUME)) && mVolumeComponent != null) {
            mVolumeComponent.dispatchDemoCommand(command, args);
        }
        if (modeChange || command.equals(COMMAND_CLOCK)) {
            dispatchDemoCommandToView(command, args, R.id.clock);
        }
        if (modeChange || command.equals(COMMAND_BATTERY)) {
            dispatchDemoCommandToView(command, args, R.id.battery);
        }
        if (modeChange || command.equals(COMMAND_STATUS)) {
            if (mDemoStatusIcons == null) {
                mDemoStatusIcons = new DemoStatusIcons(mStatusIcons, mIconSize);
            }
            mDemoStatusIcons.dispatchDemoCommand(command, args);
        }
        if (mNetworkController != null && (modeChange || command.equals(COMMAND_NETWORK))) {
            mNetworkController.dispatchDemoCommand(command, args);
        }
        if (modeChange || command.equals(COMMAND_NOTIFICATIONS)) {
            View notifications = mStatusBarView == null ? null
                    : mStatusBarView.findViewById(R.id.notification_icon_area);
            if (notifications != null) {
                String visible = args.getString("visible");
                int vis = mDemoMode && "false".equals(visible) ? View.INVISIBLE : View.VISIBLE;
                notifications.setVisibility(vis);
            }
        }
        if (command.equals(COMMAND_BARS)) {
            String mode = args.getString("mode");
            int barMode = "opaque".equals(mode) ? MODE_OPAQUE :
                    "translucent".equals(mode) ? MODE_TRANSLUCENT :
                    "semi-transparent".equals(mode) ? MODE_SEMI_TRANSPARENT :
                    "transparent".equals(mode) ? MODE_TRANSPARENT :
                    "warning".equals(mode) ? MODE_WARNING :
                    -1;
            if (barMode != -1) {
                boolean animate = true;
                if (mStatusBarView != null) {
                    mStatusBarView.getBarTransitions().transitionTo(barMode, animate);
                }
                if (mNavigationBarView != null) {
                    mNavigationBarView.getBarTransitions().transitionTo(barMode, animate);
                }
            }
        }
    }

    private void dispatchDemoCommandToView(String command, Bundle args, int id) {
        if (mStatusBarView == null) return;
        View v = mStatusBarView.findViewById(id);
        if (v instanceof DemoMode) {
            ((DemoMode)v).dispatchDemoCommand(command, args);
        }
    }

    /**
     * @return The {@link StatusBarState} the status bar is in.
     */
    public int getBarState() {
        return mState;
    }

    public void showKeyguard() {
        if (mLaunchTransitionFadingAway) {
            mNotificationPanel.animate().cancel();
            mNotificationPanel.setAlpha(1f);
            runLaunchTransitionEndRunnable();
            mLaunchTransitionFadingAway = false;
        }
        mHandler.removeMessages(MSG_LAUNCH_TRANSITION_TIMEOUT);
        setBarState(StatusBarState.KEYGUARD);
        updateKeyguardState(false /* goingToFullShade */, false /* fromShadeLocked */);
        if (!mScreenOnFromKeyguard) {

            // If the screen is off already, we need to disable touch events because these might
            // collapse the panel after we expanded it, and thus we would end up with a blank
            // Keyguard.
            mNotificationPanel.setTouchDisabled(true);
        }
        instantExpandNotificationsPanel();
        mLeaveOpenOnKeyguardHide = false;
        if (mDraggedDownRow != null) {
            mDraggedDownRow.setUserLocked(false);
            mDraggedDownRow.notifyHeightChanged();
            mDraggedDownRow = null;
        }
    }

    public boolean isCollapsing() {
        return mNotificationPanel.isCollapsing();
    }

    public void addPostCollapseAction(Runnable r) {
        mPostCollapseRunnables.add(r);
    }

    public boolean isInLaunchTransition() {
        return mNotificationPanel.isLaunchTransitionRunning()
                || mNotificationPanel.isLaunchTransitionFinished();
    }

    /**
     * Fades the content of the keyguard away after the launch transition is done.
     *
     * @param beforeFading the runnable to be run when the circle is fully expanded and the fading
     *                     starts
     * @param endRunnable the runnable to be run when the transition is done
     */
    public void fadeKeyguardAfterLaunchTransition(final Runnable beforeFading,
            Runnable endRunnable) {
        mHandler.removeMessages(MSG_LAUNCH_TRANSITION_TIMEOUT);
        mLaunchTransitionEndRunnable = endRunnable;
        Runnable hideRunnable = new Runnable() {
            @Override
            public void run() {
                mLaunchTransitionFadingAway = true;
                if (beforeFading != null) {
                    beforeFading.run();
                }
                mNotificationPanel.setAlpha(1);
                mNotificationPanel.animate()
                        .alpha(0)
                        .setStartDelay(FADE_KEYGUARD_START_DELAY)
                        .setDuration(FADE_KEYGUARD_DURATION)
                        .withLayer()
                        .withEndAction(new Runnable() {
                            @Override
                            public void run() {
                                mNotificationPanel.setAlpha(1);
                                runLaunchTransitionEndRunnable();
                                mLaunchTransitionFadingAway = false;
                            }
                        });
            }
        };
        if (mNotificationPanel.isLaunchTransitionRunning()) {
            mNotificationPanel.setLaunchTransitionEndRunnable(hideRunnable);
        } else {
            hideRunnable.run();
        }
    }

    /**
     * Starts the timeout when we try to start the affordances on Keyguard. We usually rely that
     * Keyguard goes away via fadeKeyguardAfterLaunchTransition, however, that might not happen
     * because the launched app crashed or something else went wrong.
     */
    public void startLaunchTransitionTimeout() {
        mHandler.sendEmptyMessageDelayed(MSG_LAUNCH_TRANSITION_TIMEOUT,
                LAUNCH_TRANSITION_TIMEOUT_MS);
    }

    private void onLaunchTransitionTimeout() {
        Log.w(TAG, "Launch transition: Timeout!");
        mNotificationPanel.resetViews();
    }

    private void runLaunchTransitionEndRunnable() {
        if (mLaunchTransitionEndRunnable != null) {
            Runnable r = mLaunchTransitionEndRunnable;

            // mLaunchTransitionEndRunnable might call showKeyguard, which would execute it again,
            // which would lead to infinite recursion. Protect against it.
            mLaunchTransitionEndRunnable = null;
            r.run();
        }
    }

    /**
     * @return true if we would like to stay in the shade, false if it should go away entirely
     */
    public boolean hideKeyguard() {
        boolean staying = mLeaveOpenOnKeyguardHide;
        setBarState(StatusBarState.SHADE);
        if (mLeaveOpenOnKeyguardHide) {
            mLeaveOpenOnKeyguardHide = false;
            mNotificationPanel.animateToFullShade(calculateGoingToFullShadeDelay());
            if (mDraggedDownRow != null) {
                mDraggedDownRow.setUserLocked(false);
                mDraggedDownRow = null;
            }
        } else {
            instantCollapseNotificationPanel();
        }
        updateKeyguardState(staying, false /* fromShadeLocked */);

        // Keyguard state has changed, but QS is not listening anymore. Make sure to update the tile
        // visibilities so next time we open the panel we know the correct height already.
        if (mQSPanel != null) {
            mQSPanel.refreshAllTiles();
        }
        mHandler.removeMessages(MSG_LAUNCH_TRANSITION_TIMEOUT);
        return staying;
    }

    public long calculateGoingToFullShadeDelay() {
        return mKeyguardFadingAwayDelay + mKeyguardFadingAwayDuration;
    }

    /**
     * Notifies the status bar the Keyguard is fading away with the specified timings.
     *
     * @param delay the animation delay in miliseconds
     * @param fadeoutDuration the duration of the exit animation, in milliseconds
     */
    public void setKeyguardFadingAway(long delay, long fadeoutDuration) {
        mKeyguardFadingAway = true;
        mKeyguardFadingAwayDelay = delay;
        mKeyguardFadingAwayDuration = fadeoutDuration;
        mWaitingForKeyguardExit = false;
        disable(mDisabledUnmodified, true /* animate */);
    }

    public boolean isKeyguardFadingAway() {
        return mKeyguardFadingAway;
    }

    /**
     * Notifies that the Keyguard fading away animation is done.
     */
    public void finishKeyguardFadingAway() {
        mKeyguardFadingAway = false;
    }

    private void updatePublicMode() {
        setLockscreenPublicMode(mStatusBarKeyguardViewManager.isShowing()
                && mStatusBarKeyguardViewManager.isSecure(mCurrentUserId));
    }

    private void updateKeyguardState(boolean goingToFullShade, boolean fromShadeLocked) {
        if (mState == StatusBarState.KEYGUARD) {
            mKeyguardIndicationController.setVisible(true);
            mNotificationPanel.resetViews();
            mKeyguardUserSwitcher.setKeyguard(true, fromShadeLocked);
        } else {
            mKeyguardIndicationController.setVisible(false);
            mKeyguardUserSwitcher.setKeyguard(false,
                    goingToFullShade || mState == StatusBarState.SHADE_LOCKED || fromShadeLocked);
        }
        if (mState == StatusBarState.KEYGUARD || mState == StatusBarState.SHADE_LOCKED) {
            mScrimController.setKeyguardShowing(true);
        } else {
            mScrimController.setKeyguardShowing(false);
        }
        mNotificationPanel.setBarState(mState, mKeyguardFadingAway, goingToFullShade);
        updateDozingState();
        updatePublicMode();
        updateStackScrollerState(goingToFullShade);
        updateNotifications();
        checkBarModes();
        updateCarrierLabelVisibility(false);
        updateMediaMetaData(false);
        mKeyguardMonitor.notifyKeyguardState(mStatusBarKeyguardViewManager.isShowing(),
                mStatusBarKeyguardViewManager.isSecure());
    }

    private void updateDozingState() {
        if (mState != StatusBarState.KEYGUARD && !mNotificationPanel.isDozing()) {
            return;
        }
        boolean animate = !mDozing && mDozeScrimController.isPulsing();
        mNotificationPanel.setDozing(mDozing, animate);
        mStackScroller.setDark(mDozing, animate, mScreenOnTouchLocation);
        mScrimController.setDozing(mDozing);
        mDozeScrimController.setDozing(mDozing, animate);
    }

    public void updateStackScrollerState(boolean goingToFullShade) {
        if (mStackScroller == null) return;
        boolean onKeyguard = mState == StatusBarState.KEYGUARD;
        mStackScroller.setHideSensitive(isLockscreenPublicMode(), goingToFullShade);
        mStackScroller.setDimmed(onKeyguard, false /* animate */);
        mStackScroller.setExpandingEnabled(!onKeyguard);
        ActivatableNotificationView activatedChild = mStackScroller.getActivatedChild();
        mStackScroller.setActivatedChild(null);
        if (activatedChild != null) {
            activatedChild.makeInactive(false /* animate */);
        }
    }

    public void userActivity() {
        if (mState == StatusBarState.KEYGUARD) {
            mKeyguardViewMediatorCallback.userActivity();
        }
    }

    public boolean interceptMediaKey(KeyEvent event) {
        return mState == StatusBarState.KEYGUARD
                && mStatusBarKeyguardViewManager.interceptMediaKey(event);
    }

    public boolean onMenuPressed() {
        return mState == StatusBarState.KEYGUARD && mStatusBarKeyguardViewManager.onMenuPressed();
    }

    public boolean onBackPressed() {
        if (mStatusBarKeyguardViewManager.onBackPressed()) {
            return true;
        }
        if (mNotificationPanel.isQsExpanded()) {
            if (mNotificationPanel.isQsDetailShowing()) {
                mNotificationPanel.closeQsDetail();
            } else {
                mNotificationPanel.animateCloseQs();
            }
            return true;
        }
        if (mState != StatusBarState.KEYGUARD && mState != StatusBarState.SHADE_LOCKED) {
            animateCollapsePanels();
            return true;
        }
        return false;
    }

    public boolean onSpacePressed() {
        if (mScreenOn != null && mScreenOn
                && (mState == StatusBarState.KEYGUARD || mState == StatusBarState.SHADE_LOCKED)) {
            animateCollapsePanels(
                    CommandQueue.FLAG_EXCLUDE_RECENTS_PANEL /* flags */, true /* force */);
            return true;
        }
        return false;
    }

    private void showBouncer() {
        if (mState == StatusBarState.KEYGUARD || mState == StatusBarState.SHADE_LOCKED) {
            mWaitingForKeyguardExit = mStatusBarKeyguardViewManager.isShowing();
            mStatusBarKeyguardViewManager.dismiss();
        }
    }

    private void instantExpandNotificationsPanel() {

        // Make our window larger and the panel expanded.
        makeExpandedVisible(true);
        mNotificationPanel.instantExpand();
    }

    private void instantCollapseNotificationPanel() {
        mNotificationPanel.instantCollapse();
    }

    @Override
    public void onActivated(ActivatableNotificationView view) {
        EventLogTags.writeSysuiLockscreenGesture(
                EventLogConstants.SYSUI_LOCKSCREEN_GESTURE_TAP_NOTIFICATION_ACTIVATE,
                0 /* lengthDp - N/A */, 0 /* velocityDp - N/A */);
        mKeyguardIndicationController.showTransientIndication(R.string.notification_tap_again);
        ActivatableNotificationView previousView = mStackScroller.getActivatedChild();
        if (previousView != null) {
            previousView.makeInactive(true /* animate */);
        }
        mStackScroller.setActivatedChild(view);
    }

    /**
     * @param state The {@link StatusBarState} to set.
     */
    public void setBarState(int state) {
        // If we're visible and switched to SHADE_LOCKED (the user dragged
        // down on the lockscreen), clear notification LED, vibration,
        // ringing.
        // Other transitions are covered in handleVisibleToUserChanged().
        if (state != mState && mVisible && state == StatusBarState.SHADE_LOCKED) {
            try {
                mBarService.clearNotificationEffects();
            } catch (RemoteException e) {
                // Ignore.
            }
        }
        mState = state;
        mStatusBarWindowManager.setStatusBarState(state);
    }

    @Override
    public void onActivationReset(ActivatableNotificationView view) {
        if (view == mStackScroller.getActivatedChild()) {
            mKeyguardIndicationController.hideTransientIndication();
            mStackScroller.setActivatedChild(null);
        }
    }

    public void onTrackingStarted() {
        runPostCollapseRunnables();
    }

    public void onClosingFinished() {
        runPostCollapseRunnables();
    }

    public void onUnlockHintStarted() {
        mKeyguardIndicationController.showTransientIndication(R.string.keyguard_unlock);
    }

    public void onHintFinished() {
        // Delay the reset a bit so the user can read the text.
        mKeyguardIndicationController.hideTransientIndicationDelayed(HINT_RESET_DELAY_MS);
    }

    public void onCameraHintStarted() {
        mKeyguardIndicationController.showTransientIndication(R.string.camera_hint);
    }

    public void onPhoneHintStarted() {
        mKeyguardIndicationController.showTransientIndication(R.string.phone_hint);
    }

    public void onTrackingStopped(boolean expand) {
        if (mState == StatusBarState.KEYGUARD || mState == StatusBarState.SHADE_LOCKED) {
            if (!expand && !mUnlockMethodCache.isCurrentlyInsecure()) {
                showBouncer();
            }
        }
    }

    @Override
    protected int getMaxKeyguardNotifications() {
        return mKeyguardMaxNotificationCount;
    }

    public NavigationBarView getNavigationBarView() {
        return mNavigationBarView;
    }

    // ---------------------- DragDownHelper.OnDragDownListener ------------------------------------

    @Override
    public boolean onDraggedDown(View startingChild, int dragLengthY) {
        if (hasActiveNotifications()) {
            EventLogTags.writeSysuiLockscreenGesture(
                    EventLogConstants.SYSUI_LOCKSCREEN_GESTURE_SWIPE_DOWN_FULL_SHADE,
                    (int) (dragLengthY / mDisplayMetrics.density),
                    0 /* velocityDp - N/A */);

            // We have notifications, go to locked shade.
            goToLockedShade(startingChild);
            return true;
        } else {

            // No notifications - abort gesture.
            return false;
        }
    }

    @Override
    public void onDragDownReset() {
        mStackScroller.setDimmed(true /* dimmed */, true /* animated */);
    }

    @Override
    public void onThresholdReached() {
        mStackScroller.setDimmed(false /* dimmed */, true /* animate */);
    }

    @Override
    public void onTouchSlopExceeded() {
        mStackScroller.removeLongPressCallback();
    }

    @Override
    public void setEmptyDragAmount(float amount) {
        mNotificationPanel.setEmptyDragAmount(amount);
    }

    /**
     * If secure with redaction: Show bouncer, go to unlocked shade.
     *
     * <p>If secure without redaction or no security: Go to {@link StatusBarState#SHADE_LOCKED}.</p>
     *
     * @param expandView The view to expand after going to the shade.
     */
    public void goToLockedShade(View expandView) {
        ExpandableNotificationRow row = null;
        if (expandView instanceof ExpandableNotificationRow) {
            row = (ExpandableNotificationRow) expandView;
            row.setUserExpanded(true);
        }
        boolean fullShadeNeedsBouncer = !userAllowsPrivateNotificationsInPublic(mCurrentUserId)
                || !mShowLockscreenNotifications;
        if (isLockscreenPublicMode() && fullShadeNeedsBouncer) {
            mLeaveOpenOnKeyguardHide = true;
            showBouncer();
            mDraggedDownRow = row;
        } else {
            mNotificationPanel.animateToFullShade(0 /* delay */);
            setBarState(StatusBarState.SHADE_LOCKED);
            updateKeyguardState(false /* goingToFullShade */, false /* fromShadeLocked */);
            if (row != null) {
                row.setUserLocked(false);
            }
        }
    }

    /**
     * Goes back to the keyguard after hanging around in {@link StatusBarState#SHADE_LOCKED}.
     */
    public void goToKeyguard() {
        if (mState == StatusBarState.SHADE_LOCKED) {
            mStackScroller.onGoToKeyguard();
            setBarState(StatusBarState.KEYGUARD);
            updateKeyguardState(false /* goingToFullShade */, true /* fromShadeLocked*/);
        }
    }

    /**
     * @return a ViewGroup that spans the entire panel which contains the quick settings
     */
    public ViewGroup getQuickSettingsOverlayParent() {
        return mNotificationPanel;
    }

    public long getKeyguardFadingAwayDelay() {
        return mKeyguardFadingAwayDelay;
    }

    public long getKeyguardFadingAwayDuration() {
        return mKeyguardFadingAwayDuration;
    }

    public LinearLayout getSystemIcons() {
        return mSystemIcons;
    }

    public LinearLayout getSystemIconArea() {
        return mSystemIconArea;
    }

    @Override
    public void setBouncerShowing(boolean bouncerShowing) {
        super.setBouncerShowing(bouncerShowing);
        disable(mDisabledUnmodified, true /* animate */);
    }

    public void onScreenTurnedOff() {
        mScreenOnFromKeyguard = false;
        mScreenOnComingFromTouch = false;
        mScreenOnTouchLocation = null;
        mStackScroller.setAnimationsEnabled(false);
        updateVisibleToUser();
    }

    public void onScreenTurnedOn() {
        mScreenOnFromKeyguard = true;
        mStackScroller.setAnimationsEnabled(true);
        mNotificationPanel.onScreenTurnedOn();
        mNotificationPanel.setTouchDisabled(false);
        updateVisibleToUser();
    }

    /**
     * This handles long-press of both back and recents.  They are
     * handled together to capture them both being long-pressed
     * at the same time to exit screen pinning (lock task).
     *
     * When accessibility mode is on, only a long-press from recents
     * is required to exit.
     *
     * In all other circumstances we try to pass through long-press events
     * for Back, so that apps can still use it.  Which can be from two things.
     * 1) Not currently in screen pinning (lock task).
     * 2) Back is long-pressed without recents.
     */
    private void handleLongPressBackRecents(View v) {
        try {
            boolean sendBackLongPress = false;
            IActivityManager activityManager = ActivityManagerNative.getDefault();
            boolean isAccessiblityEnabled = mAccessibilityManager.isEnabled();
            if (activityManager.isInLockTaskMode() && !isAccessiblityEnabled) {
                long time = System.currentTimeMillis();
                // If we recently long-pressed the other button then they were
                // long-pressed 'together'
                if ((time - mLastLockToAppLongPress) < LOCK_TO_APP_GESTURE_TOLERENCE) {
                    activityManager.stopLockTaskModeOnCurrent();
                    // When exiting refresh disabled flags.
                    mNavigationBarView.setDisabledFlags(mDisabled, true);
                } else if ((v.getId() == R.id.back)
                        && !mNavigationBarView.getRecentsButton().isPressed()) {
                    // If we aren't pressing recents right now then they presses
                    // won't be together, so send the standard long-press action.
                    sendBackLongPress = true;
                }
                mLastLockToAppLongPress = time;
            } else {
                // If this is back still need to handle sending the long-press event.
                if (v.getId() == R.id.home&&mContext.getResources().getConfiguration().enableMultiWindow()) {
					LOGD("wintask Longpress home button for clear tasks!");
					ClearRunningTasks();
					if(appslist!=null && appslist.size() > 0){
					 appslist.clear();  
		             listadapter.notifyDataSetChanged();
				  }
                } else if (v.getId() == R.id.back) {
                    sendBackLongPress = true;
                } else if (isAccessiblityEnabled && activityManager.isInLockTaskMode()) {
                    // When in accessibility mode a long press that is recents (not back)
                    // should stop lock task.
                    activityManager.stopLockTaskModeOnCurrent();
                    // When exiting refresh disabled flags.
                    mNavigationBarView.setDisabledFlags(mDisabled, true);
                }
            }
            if (sendBackLongPress) {
                KeyButtonView keyButtonView = (KeyButtonView) v;
                keyButtonView.sendEvent(KeyEvent.ACTION_DOWN, KeyEvent.FLAG_LONG_PRESS);
                keyButtonView.sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_LONG_CLICKED);
            }
        } catch (RemoteException e) {
            Log.d(TAG, "Unable to reach activity manager", e);
        }
    }

    // Recents

    @Override
    protected void showRecents(boolean triggeredFromAltTab) {
        // Set the recents visibility flag
        mSystemUiVisibility |= View.RECENT_APPS_VISIBLE;
        notifyUiVisibilityChanged(mSystemUiVisibility);
        super.showRecents(triggeredFromAltTab);
    }

    @Override
    protected void hideRecents(boolean triggeredFromAltTab, boolean triggeredFromHomeKey) {
        // Unset the recents visibility flag
        mSystemUiVisibility &= ~View.RECENT_APPS_VISIBLE;
        notifyUiVisibilityChanged(mSystemUiVisibility);
        super.hideRecents(triggeredFromAltTab, triggeredFromHomeKey);
    }

    @Override
    protected void toggleRecents() {
        // Toggle the recents visibility flag
        mSystemUiVisibility ^= View.RECENT_APPS_VISIBLE;
        notifyUiVisibilityChanged(mSystemUiVisibility);
        super.toggleRecents();
    }

    @Override
    public void onVisibilityChanged(boolean visible) {
        // Update the recents visibility flag
        if (visible) {
            mSystemUiVisibility |= View.RECENT_APPS_VISIBLE;
        } else {
            mSystemUiVisibility &= ~View.RECENT_APPS_VISIBLE;
        }
        notifyUiVisibilityChanged(mSystemUiVisibility);
    }

    @Override
    public void showScreenPinningRequest() {
        if (mKeyguardMonitor.isShowing()) {
            // Don't allow apps to trigger this from keyguard.
            return;
        }
        // Show screen pinning request, since this comes from an app, show 'no thanks', button.
        showScreenPinningRequest(true);
    }

    public void showScreenPinningRequest(boolean allowCancel) {
        mScreenPinningRequest.showPrompt(allowCancel);
    }

    public boolean hasActiveNotifications() {
        return !mNotificationData.getActiveNotifications().isEmpty();
    }

    public void wakeUpIfDozing(long time, MotionEvent event) {
        if (mDozing && mDozeScrimController.isPulsing()) {
            PowerManager pm = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
            pm.wakeUp(time);
            mScreenOnComingFromTouch = true;
            mScreenOnTouchLocation = new PointF(event.getX(), event.getY());
            mNotificationPanel.setTouchDisabled(false);
        }
    }

    private final class ShadeUpdates {
        private final ArraySet<String> mVisibleNotifications = new ArraySet<String>();
        private final ArraySet<String> mNewVisibleNotifications = new ArraySet<String>();

        public void check() {
            mNewVisibleNotifications.clear();
            ArrayList<Entry> activeNotifications = mNotificationData.getActiveNotifications();
            for (int i = 0; i < activeNotifications.size(); i++) {
                final Entry entry = activeNotifications.get(i);
                final boolean visible = entry.row != null
                        && entry.row.getVisibility() == View.VISIBLE;
                if (visible) {
                    mNewVisibleNotifications.add(entry.key + entry.notification.getPostTime());
                }
            }
            final boolean updates = !mVisibleNotifications.containsAll(mNewVisibleNotifications);
            mVisibleNotifications.clear();
            mVisibleNotifications.addAll(mNewVisibleNotifications);

            // We have new notifications
            if (updates && mDozeServiceHost != null) {
                mDozeServiceHost.fireNewNotifications();
            }
        }
    }

    private final class DozeServiceHost implements DozeHost {
        // Amount of time to allow to update the time shown on the screen before releasing
        // the wakelock.  This timeout is design to compensate for the fact that we don't
        // currently have a way to know when time display contents have actually been
        // refreshed once we've finished rendering a new frame.
        private static final long PROCESSING_TIME = 500;

        private final ArrayList<Callback> mCallbacks = new ArrayList<Callback>();
        private final H mHandler = new H();

        // Keeps the last reported state by fireNotificationLight.
        private boolean mNotificationLightOn;

        @Override
        public String toString() {
            return "PSB.DozeServiceHost[mCallbacks=" + mCallbacks.size() + "]";
        }

        public void firePowerSaveChanged(boolean active) {
            for (Callback callback : mCallbacks) {
                callback.onPowerSaveChanged(active);
            }
        }

        public void fireBuzzBeepBlinked() {
            for (Callback callback : mCallbacks) {
                callback.onBuzzBeepBlinked();
            }
        }

        public void fireNotificationLight(boolean on) {
            mNotificationLightOn = on;
            for (Callback callback : mCallbacks) {
                callback.onNotificationLight(on);
            }
        }

        public void fireNewNotifications() {
            for (Callback callback : mCallbacks) {
                callback.onNewNotifications();
            }
        }

        @Override
        public void addCallback(@NonNull Callback callback) {
            mCallbacks.add(callback);
        }

        @Override
        public void removeCallback(@NonNull Callback callback) {
            mCallbacks.remove(callback);
        }

        @Override
        public void startDozing(@NonNull Runnable ready) {
            mHandler.obtainMessage(H.MSG_START_DOZING, ready).sendToTarget();
        }

        @Override
        public void pulseWhileDozing(@NonNull PulseCallback callback, int reason) {
            mHandler.obtainMessage(H.MSG_PULSE_WHILE_DOZING, reason, 0, callback).sendToTarget();
        }

        @Override
        public void stopDozing() {
            mHandler.obtainMessage(H.MSG_STOP_DOZING).sendToTarget();
        }

        @Override
        public boolean isPowerSaveActive() {
            return mBatteryController != null && mBatteryController.isPowerSave();
        }

        @Override
        public boolean isNotificationLightOn() {
            return mNotificationLightOn;
        }

        private void handleStartDozing(@NonNull Runnable ready) {
            if (!mDozing) {
                mDozing = true;
                DozeLog.traceDozing(mContext, mDozing);
                updateDozingState();
            }
            ready.run();
        }

        private void handlePulseWhileDozing(@NonNull PulseCallback callback, int reason) {
            mDozeScrimController.pulse(callback, reason);
        }

        private void handleStopDozing() {
            if (mDozing) {
                mDozing = false;
                DozeLog.traceDozing(mContext, mDozing);
                updateDozingState();
            }
        }
         private final class H extends Handler {
            private static final int MSG_START_DOZING = 1;
            private static final int MSG_PULSE_WHILE_DOZING = 2;
            private static final int MSG_STOP_DOZING = 3;

            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_START_DOZING:
                        handleStartDozing((Runnable) msg.obj);
                        break;
                    case MSG_PULSE_WHILE_DOZING:
                        handlePulseWhileDozing((PulseCallback) msg.obj, msg.arg1);
                        break;
                    case MSG_STOP_DOZING:
                        handleStopDozing();
                        break;
                }
            }
        }
    }
    private View.OnClickListener mSimSwitchListener = new View.OnClickListener() {
        public void onClick(View v) {
            int value = 0;
            if (v.getId() == mSwitchSim1Button.getId()) {
                value = 0;
            } else if (v.getId() == mSwitchSim2Button.getId()) {
                value = 1;
            } else if (v.getId() == mSwitchAskButton.getId()) {
                value = -1;
            }
            switch (mSwitchType) {
                case StatusBarManager.SWITCH_VOICE_CALL:
                    mPhoneCards.setCall(value);
                    break;
                case StatusBarManager.SWITCH_MESSAGE:
                    mPhoneCards.setSms(value);
                    break;
                case StatusBarManager.SWITCH_VIDEO_CALL:
                default:
                    break;
            }

            updateSimSwitchUi();
            animateCollapsePanels();
        }
    };

    // indicate the SimSwitch UI showing or not.
    private static boolean mSimSwitchUi = false;

    public static boolean getSimSwitch() {
        return mSimSwitchUi;
    }

    @Override
    public void showSimSwitchUi(int type) {
        int num = mPhoneCards.getCardsNum();
        if( num <= 1 ) {
            hideSimSwitchUi();
            return;
        }
        mSwitchType = type;

        updateSimSwitchUi();
        mSimSwitchUi = true;
        mSimSwitchContainer.setVisibility(View.VISIBLE);
    }

    @Override
    public void hideSimSwitchUi() {
        mSimSwitchNotification.setVisibility(View.GONE);
        mSimSwitchContainer.setVisibility(View.GONE);
        mSimSwitchUi = false;
    }

    private void updateSimSwitchUi() {
        boolean selected = false;
        final CardInfo card = mPhoneCards.getDefault(mSwitchType);
        final CardInfo sim1 = mPhoneCards.getVirtualCard(0);
        final CardInfo sim2 = mPhoneCards.getVirtualCard(1);
        if (sim1!=null) {
            selected = (sim1.getId()==card.getId());
            mSwitchSim1Button.setImageBitmap(PhoneCards.selectedIcon(sim1.getIcon(), selected));
            mSwitchSim1Button.invalidate();
            mSim1Container.setVisibility(View.VISIBLE);
        } else {
            mSim1Container.setVisibility(View.GONE);
        }

        if (sim2!=null) {
            selected = (sim2.getId()==card.getId());
            mSwitchSim2Button.setImageBitmap(PhoneCards.selectedIcon(sim2.getIcon(), selected));
            mSwitchSim2Button.invalidate();
            mSim2Container.setVisibility(View.VISIBLE);
        } else {
            mSim2Container.setVisibility(View.GONE);
        }
        if (sim1!=null||sim2!=null) {
            final CardInfo ask = mPhoneCards.getVirtualCard(2);
            selected = (ask.getId()==card.getId());
            mSwitchAskButton.setImageBitmap(PhoneCards.selectedIcon(ask.getIcon(), selected));
            mSwitchAskButton.invalidate();
            mAskContainer.setVisibility(View.VISIBLE);
        } else {
            mAskContainer.setVisibility(View.GONE);
        }

        // Display the notification text and color
        mSimSwitchNotificationText.setText(card.getName());
        mSimSwitchNotificationText.setTextColor(card.getColor());
        mSimSwitchNotification.setVisibility(View.VISIBLE);
    }

	private class MyGestureDetectorListener  extends SimpleOnGestureListener{
		@Override
		public boolean onScroll(MotionEvent e1, MotionEvent e2,
				float distanceX, float distanceY) {
			int startX = (int)e1.getX();
			int startY = (int)e1.getY();
			int currentX = (int)e2.getRawX();
			int currentY = (int)e2.getRawY();
			mHSCParams.x = currentX - startX;
			mHSCParams.y = currentY - startY;
			//portarial
			if(mHSCParams.width == -1){
				int height = wm.getDefaultDisplay().getHeight() - MAX_RAN_MULCON;
				mHSCParams.x = 0;
				if(mHSCParams.y < MAX_RAN_MULCON){
					mHSCParams.y = MAX_RAN_MULCON;
				}else if(mHSCParams.y > height){
					mHSCParams.y = height;
				}
			}
			//lanscape
			if(mHSCParams.height == -1){
				int width = wm.getDefaultDisplay().getWidth() - MAX_RAN_MULCON;
				mHSCParams.y = 0;
				if(mHSCParams.x < MAX_RAN_MULCON){
					mHSCParams.x = MAX_RAN_MULCON;
				}else if(mHSCParams.x > width){
					mHSCParams.x = width;
				}
			}
			if(mHSContainer!=null){
				wm.updateViewLayout(mHSContainer, mHSCParams);
			}
			return true;
		}
    }
	
	private class MyGestureOfFourScreenWDetectorListener  extends SimpleOnGestureListener{
		@Override
		public boolean onScroll(MotionEvent e1, MotionEvent e2,
				float distanceX, float distanceY) {
			LOGD("---------MyGestureOfFourScreenWDetectorListener-------onScroll------------");
			if(mFourScreenWCParams == null){
				return true;
			}
			int startX = (int)e1.getX();
			int startY = (int)e1.getY();
			int currentX = (int)e2.getRawX();
			int currentY = (int)e2.getRawY();
			mFourScreenWCParams.y = currentY - startY;
			if(mEdgeRestriction){
				int height = wm.getDefaultDisplay().getHeight() - MAX_RAN_MULCON;
				mFourScreenWCParams.x = 0;
				if(mFourScreenWCParams.y < MAX_RAN_MULCON){
					mFourScreenWCParams.y = MAX_RAN_MULCON;
				}else if(mFourScreenWCParams.y > height){
					mFourScreenWCParams.y = height;
				}
			}else{
				if(mFourScreenWCParams.y < mStatusBarHeight){
					mFourScreenWCParams.y = mStatusBarHeight;
				}else if(mFourScreenWCParams.y+MUL_CON_POS*2 > wm.getDefaultDisplay().getHeight()){
					mFourScreenWCParams.y = wm.getDefaultDisplay().getHeight()-MUL_CON_POS*2;
				}
			}
			updateCenterBtnView();
			updateCircleMenu();
			updateFourScreenControllers(null);
			return true;
		}
    }
	
	private class MyGestureOfFourScreenHDetectorListener  extends SimpleOnGestureListener{
		@Override
		public boolean onScroll(MotionEvent e1, MotionEvent e2,
				float distanceX, float distanceY) {
			if(mFourScreenHCParams == null){
				return true;
			}
			int startX = (int)e1.getX();
			int startY = (int)e1.getY();
			int currentX = (int)e2.getRawX();
			int currentY = (int)e2.getRawY();
			mFourScreenHCParams.x = currentX - startX;
			if(mEdgeRestriction){
				int width = wm.getDefaultDisplay().getWidth() - MAX_RAN_MULCON;
				mFourScreenHCParams.y = 0;
				if(mFourScreenHCParams.x < MAX_RAN_MULCON){
					mFourScreenHCParams.x = MAX_RAN_MULCON;
				}else if(mFourScreenHCParams.x > width){
					mFourScreenHCParams.x = width;
				}
			}else{
				if(mFourScreenHCParams.x < 0){
					mFourScreenHCParams.x = 0;
				}else if(mFourScreenHCParams.x+MUL_CON_POS*2 > wm.getDefaultDisplay().getWidth()){
					mFourScreenHCParams.x = wm.getDefaultDisplay().getWidth()-MUL_CON_POS*2;
				}
			}
			updateCenterBtnView();
			updateCircleMenu();
			updateFourScreenControllers(null);
			return true;
		}
    }
	
	private void updateCenterBtnView(){
		if(mCenterBtnParams != null){
			if(mFourScreenHCParams != null)
				mCenterBtnParams.x = mFourScreenHCParams.x + MUL_CON_POS - CENTER_BTN_WIDTH/2;
			if(mFourScreenWCParams != null)
				mCenterBtnParams.y = mFourScreenWCParams.y + MUL_CON_POS - CENTER_BTN_HEIGHT/2;
			if (wm != null && mCenterBtnContainer.getParent() != null) {
				wm.updateViewLayout(mCenterBtnContainer, mCenterBtnParams);
			}
		}
	}

	private class CenterBtnGestureDetectorListener extends
			SimpleOnGestureListener {
		@Override
		public boolean onScroll(MotionEvent e1, MotionEvent e2,
				float distanceX, float distanceY) {
			Log.v("zjy","-----------onScroll-----------------");
			if(isCenterBtnClick){
				CENTER_BTN_WIDTH =  CENTER_BTN_WIDTH_VALUE ;
				CENTER_BTN_HEIGHT =	CENTER_BTN_HEIGHT_VALUE ;
				mCenterBtnView.setLayoutParams(new LinearLayout.LayoutParams(CENTER_BTN_WIDTH,CENTER_BTN_HEIGHT));
				mCenterBtnView.setBackgroundResource(R.drawable.center_btn_pressed);
				mCenterBtnParams.x -= (CENTER_BTN_WIDTH - CENTER_BTN_WIDTH_VALUE)/2;
				mCenterBtnParams.y -= (CENTER_BTN_HEIGHT - CENTER_BTN_HEIGHT_VALUE)/2;
				updateCenterBtnView();
				isCenterBtnClick = false;
			}
			int startX = (int) e1.getX();
			int startY = (int) e1.getY();
			int currentX = (int) e2.getRawX();
			int currentY = (int) e2.getRawY();
			
			if(mFourScreenWCParams != null && mFourScreenHCParams != null){
				mFourScreenWCParams.y = currentY - startY;
				mFourScreenHCParams.x = currentX - startX;
				if(mEdgeRestriction){
					int height = wm.getDefaultDisplay().getHeight() - MAX_RAN_MULCON;
					mFourScreenWCParams.x = 0;
					if(mFourScreenWCParams.y < MAX_RAN_MULCON){
						mFourScreenWCParams.y = MAX_RAN_MULCON;
					}else if(mFourScreenWCParams.y > height){
						mFourScreenWCParams.y = height;
					}
					int width = wm.getDefaultDisplay().getWidth() - MAX_RAN_MULCON;
					mFourScreenHCParams.y =0;
					if(mFourScreenHCParams.x < MAX_RAN_MULCON){
						mFourScreenHCParams.x = MAX_RAN_MULCON;
					}else if(mFourScreenHCParams.x > width){
						mFourScreenHCParams.x = width;
					}
				}else{
					if(mFourScreenHCParams.x < 0){
						mFourScreenHCParams.x = 0;
					}else if(mFourScreenHCParams.x+MUL_CON_POS*2 > wm.getDefaultDisplay().getWidth()){
						mFourScreenHCParams.x = wm.getDefaultDisplay().getWidth()-MUL_CON_POS*2;
					}
					if(mFourScreenWCParams.y < mStatusBarHeight){
						mFourScreenWCParams.y = mStatusBarHeight;
					}else if(mFourScreenWCParams.y+MUL_CON_POS*2 > wm.getDefaultDisplay().getHeight()){
						mFourScreenWCParams.y = wm.getDefaultDisplay().getHeight()-MUL_CON_POS*2;
					}
				}
				updateCenterBtnView();
				updateCircleMenu();
				updateFourScreenControllers(null);
			}else{
				mCenterBtnParams.x = currentX - startX;
				mCenterBtnParams.y = currentY - startY;
				if(mCenterBtnParams.x < 0){
					mCenterBtnParams.x = 0;
				}else if(mCenterBtnParams.x+CENTER_BTN_WIDTH > wm.getDefaultDisplay().getWidth()){
					mCenterBtnParams.x = wm.getDefaultDisplay().getWidth()-CENTER_BTN_WIDTH;
				}
				if(mCenterBtnParams.y < mStatusBarHeight){
					mCenterBtnParams.y = mStatusBarHeight;
				}else if(mCenterBtnParams.y+CENTER_BTN_HEIGHT > wm.getDefaultDisplay().getHeight()){
					mCenterBtnParams.y = wm.getDefaultDisplay().getHeight()-CENTER_BTN_HEIGHT;
				}
				if (wm != null && mCenterBtnContainer.getParent() != null) {
					wm.updateViewLayout(mCenterBtnContainer, mCenterBtnParams);
				}
				if(mCircleMenuParams != null){
					mCircleMenuParams.x = mCenterBtnParams.x + CENTER_BTN_WIDTH_VALUE/2 - FourScreenCircleMenuView.WIDTH/2;
					mCircleMenuParams.y = mCenterBtnParams.y +  CENTER_BTN_HEIGHT_VALUE/2 - FourScreenCircleMenuView.HEIGHT/2;
				}
				if(mFourScreenCircleMenuView!=null && mFourScreenCircleMenuView.getParent()!=null){
					removeViewHandler.removeMessages(remove_view_msg);
					removeViewHandler.removeMessages(send_hide_view_msg);
					wm.updateViewLayout(mFourScreenCircleMenuView, mCircleMenuParams);
					if(mFourScreenCircleMenuView != null){
						mFourScreenCircleMenuView.show();
					}
					removeViewHandler.sendEmptyMessageDelayed(send_hide_view_msg, CIRCLE_MENU_HIDE_DELAY);
				}
			}
			
			return true;
		}

		@Override
		public boolean onSingleTapConfirmed(MotionEvent e) {
			LOGD("onSingleTapUp");
			if(ONE_LEVEL_MENU){
				if(mFourScreenCircleMenuView!=null){
					if(mFourScreenCircleMenuView.getParent() == null){
						showCircleMenu();
					}else{
						hideCircleMenu();
					}
				}
			}
			return true;
		}

		@Override
		public void onLongPress(MotionEvent e) {
			LOGD("onLongPress");
			if(ONE_LEVEL_MENU){
				if(mFourScreenCircleMenuView!=null){
					if(mFourScreenCircleMenuView.getParent() == null){
						showCircleMenu();
					}else{
						hideCircleMenu();
					}
				}
			}
		}
	}
	
	private class MultiModeGestureDetectorListener extends
			SimpleOnGestureListener {
		@Override
		public boolean onScroll(MotionEvent e1, MotionEvent e2,
				float distanceX, float distanceY) {
			int startX = (int) e1.getX();
			int startY = (int) e1.getY();
			int currentX = (int) e2.getRawX();
			int currentY = (int) e2.getRawY();
			mMultiModeParams.x = currentX - startX;
			mMultiModeParams.y = currentY - startY;
			if (multiModeContainer.getParent() != null) {
				wm.updateViewLayout(multiModeContainer,mMultiModeParams);
			}
			return true;
		}

		@Override
		public boolean onSingleTapConfirmed(MotionEvent e) {
			LOGD("onSingletapConfirm");
			// TODO Auto-generated method stub
			LOGD("onSingleTapUp");
			if (multiModeContainer.getParent() != null) {
				int mode = Settings.System.getInt(
						mContext.getContentResolver(),
						Settings.System.MULITI_WINDOW_MODE, 0);
				Log.d("turing---->", "multiwindow mode  used=" + mode);
				//Settings.System.putInt(mContext.getContentResolver(),
				//		Settings.System.MULITI_WINDOW_MODE, (mode + 1) % 3);
                                if(mode == 3){
					Settings.System.putInt(mContext.getContentResolver(),
							Settings.System.MULITI_WINDOW_MODE, 4);
				}else{
					Settings.System.putInt(mContext.getContentResolver(),
						Settings.System.MULITI_WINDOW_MODE, 3);
				}
				setMultiModeView();
			}
			return true;
		}

		@Override
		public void onLongPress(MotionEvent e) {
			LOGD("onLongPress");
		}
	}
	
	private void changeMultiModeViewDrawable(MotionEvent event){
		setMultiModeView();
	}
	
	private int getMultiMode(){
		return Settings.System.getInt(mContext.getContentResolver(),
    			Settings.System.MULITI_WINDOW_MODE, Settings.System.MULITI_WINDOW_FULL_SCREEN_MODE);
	}
	
	private void setMultiModeView(){
		int res = -1;
		int mode = Settings.System.getInt(mContext.getContentResolver(),
    			Settings.System.MULITI_WINDOW_MODE, Settings.System.MULITI_WINDOW_FULL_SCREEN_MODE);
		if(mode == Settings.System.MULITI_WINDOW_FULL_SCREEN_MODE){
			
		}else if(mode == Settings.System.MULITI_WINDOW_HALF_SCREEN_MODE){
			res = R.drawable.compose_normal;
		}else if(mode == Settings.System.MULITI_WINDOW_FOUR_SCREEN_MODE){
			res = R.drawable.stretch_normal;
		}
		if(mMultiModeView != null){
			if(res != -1){
				mMultiModeView.setImageResource(res);
			}else{
				mMultiModeView.setImageBitmap(null);
			}
		}
	}
	
	private void addCircleMenuWindow(){
		if(mFourScreenCircleMenuView == null){
			mFourScreenCircleMenuView = new FourScreenCircleMenuView(mContext);
			mFourScreenCircleMenuView.setBtnClickCallBack(this);
			mFourScreenCircleMenuView.setOnTouchListener(new View.OnTouchListener() {
				@Override
				public boolean onTouch(View arg0, MotionEvent arg1) {
					Log.v("zjy", "------mFourScreenCircleMenuView onTouch-------arg1:"+arg1);
					hideCircleMenu();
					return true;
				}
			});
		}
		if(mCircleMenuParams == null){
			mCircleMenuParams = new WindowManager.LayoutParams();
			mCircleMenuParams.type=WindowManager.LayoutParams.TYPE_MULTIWINDOW_FOURSCREEN_CENTER_BUTTON;//2002|WindowManager.LayoutParams.TYPE_SYSTEM_ALERT ;
			mCircleMenuParams.format=PixelFormat.TRANSLUCENT;
			mCircleMenuParams.flags|=8;
			mCircleMenuParams.flags |=WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
			mCircleMenuParams.flags |= WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;
			mCircleMenuParams.width = FourScreenCircleMenuView.WIDTH;
			mCircleMenuParams.height = FourScreenCircleMenuView.HEIGHT;
			mCircleMenuParams.x = mContext.getResources().getDisplayMetrics().widthPixels/2-FourScreenCircleMenuView.WIDTH/2;
			mCircleMenuParams.y = (mContext.getResources().getDisplayMetrics().heightPixels+mStatusBarHeight)/2 -FourScreenCircleMenuView.HEIGHT/2;
			mCircleMenuParams.setTitle("CircleMenu");
			mCircleMenuParams.gravity=Gravity.LEFT|Gravity.TOP;
		}
	}
	
	private void addCenterBtnWindow(){
		if(mCenterBtnContainer == null){
			mCenterBtnContainer = new LinearLayout(mContext);
			mCenterBtnView = new ImageView(mContext);
			mCenterBtnView.setLayoutParams(new LinearLayout.LayoutParams(CENTER_BTN_WIDTH,CENTER_BTN_HEIGHT));
			mCenterBtnView.setBackgroundResource(R.drawable.center_btn_normal);
			mCenterBtnView.setScaleType(ScaleType.CENTER);
			mCenterBtnContainer.addView(mCenterBtnView);
			mCenterBtnContainer.setFocusable(true);
			mCenterBtnContainer.setClickable(true);
			mCenterBtnContainer.setOnTouchListener(new OnTouchListener() {		
				@Override
				public boolean onTouch(View v, MotionEvent event) {
							Log.v("zjy", "-------mCenterBtnContainer---onTouch-----------------event.getAction():"+event.getAction());
							switch(event.getAction()){
							case MotionEvent.ACTION_CANCEL:
							case MotionEvent.ACTION_UP:
								Log.v("zjy", "-------ACTION_UP------------CENTER_BTN_WIDTH:"+CENTER_BTN_WIDTH +",CENTER_BTN_WIDTH_VALUE:"+CENTER_BTN_WIDTH_VALUE);
								
								AnimationSet set = new AnimationSet(true);
								ScaleAnimation animation =new ScaleAnimation(1.0f, 0.5f, 1.0f, 0.5f,
										Animation.RELATIVE_TO_SELF, 0.5f, Animation.RELATIVE_TO_SELF, 0.5f);
								set.addAnimation(animation);
								set.setDuration(300);
								set.setAnimationListener(new Animation.AnimationListener() {
									@Override
									public void onAnimationStart(Animation animation) {
									}

									@Override
									public void onAnimationRepeat(Animation animation) {
									}

									@Override
									public void onAnimationEnd(Animation animation) {
										mCenterBtnParams.x += (CENTER_BTN_WIDTH - CENTER_BTN_WIDTH_VALUE)/2;
										mCenterBtnParams.y += (CENTER_BTN_HEIGHT - CENTER_BTN_HEIGHT_VALUE)/2;
										CENTER_BTN_WIDTH =  CENTER_BTN_WIDTH_VALUE;
										CENTER_BTN_HEIGHT =	CENTER_BTN_HEIGHT_VALUE;
										mCenterBtnView.setLayoutParams(new LinearLayout.LayoutParams(CENTER_BTN_WIDTH,CENTER_BTN_HEIGHT));
										mCenterBtnView.setBackgroundResource(R.drawable.center_btn_normal);
										updateCenterBtnView();
									}
								});
								mCenterBtnView.startAnimation(set);
								updateFourScreenSettingsSystem(LAND_CONTROL_TYPE);
								if(mFourScreenWController != null)
									mFourScreenWController.setBackgroundResource(R.drawable.control_bg_normal);	
								updateFourScreenSettingsSystem(PORT_CONTROL_TYPE);
								if(mFourScreenHController != null)
									mFourScreenHController.setBackgroundResource(R.drawable.control_bg_normal);
								break;
							case MotionEvent.ACTION_MOVE:
								break;
							case MotionEvent.ACTION_DOWN:
								isCenterBtnClick = true;
								if(mFourScreenWController != null)
									mFourScreenWController.setBackgroundResource(R.drawable.control_bg_pressed);
								if(mFourScreenHController != null)
									mFourScreenHController.setBackgroundResource(R.drawable.control_bg_pressed);
								break;
							}
							mCenterBtnDector.onTouchEvent(event);
							return false;
						}
					});
		}
		if(mCenterBtnParams == null){
			mCenterBtnParams = new WindowManager.LayoutParams();
			mCenterBtnParams.type=WindowManager.LayoutParams.TYPE_MULTIWINDOW_FOURSCREEN_CENTER_BUTTON;//2002|WindowManager.LayoutParams.TYPE_SYSTEM_ALERT ;
			mCenterBtnParams.format=PixelFormat.TRANSLUCENT;
			mCenterBtnParams.flags|=8;
			mCenterBtnParams.flags |=WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
			mCenterBtnParams.width = WindowManager.LayoutParams.WRAP_CONTENT;
			mCenterBtnParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
			mCenterBtnParams.x = mContext.getResources().getDisplayMetrics().widthPixels/2 - CENTER_BTN_WIDTH/2;
			mCenterBtnParams.y = (mContext.getResources().getDisplayMetrics().heightPixels+mStatusBarHeight)/2 - CENTER_BTN_HEIGHT/2;
			mCenterBtnParams.windowAnimations = 0;
			mCenterBtnParams.setTitle("CenterViewButton");
			mCenterBtnParams.gravity=Gravity.LEFT|Gravity.TOP;

		}
		if(mCenterBtnDector == null){
			mCenterBtnDector = new GestureDetector(new CenterBtnGestureDetectorListener());
		}
	}
	
	private void showCircleMenu(){
		if(!ONE_LEVEL_MENU){
			return;
		}
		if(mFourScreenCircleMenuView!=null && mFourScreenCircleMenuView.getParent()==null){
			wm.addView(mFourScreenCircleMenuView, mCircleMenuParams);
		}
		updateCircleMenu();
	}
	
	private void updateCircleMenu(){
		if(!ONE_LEVEL_MENU){
			return;
		}
		if(mCircleMenuParams != null && mFourScreenHCParams != null){
			mCircleMenuParams.x = mFourScreenHCParams.x + MUL_CON_POS-FourScreenCircleMenuView.WIDTH/2;
			mCircleMenuParams.y = mFourScreenWCParams.y + MUL_CON_POS-FourScreenCircleMenuView.HEIGHT/2;
		}else if(mCircleMenuParams != null && mCenterBtnParams != null){
			mCircleMenuParams.x = mCenterBtnParams.x + MUL_CON_POS-FourScreenCircleMenuView.WIDTH/2;
			mCircleMenuParams.y = mCenterBtnParams.y + MUL_CON_POS-FourScreenCircleMenuView.HEIGHT/2;
		}
		if(mFourScreenCircleMenuView!=null && mFourScreenCircleMenuView.getParent()!=null){
			removeViewHandler.removeMessages(remove_view_msg);
			removeViewHandler.removeMessages(send_hide_view_msg);
			wm.updateViewLayout(mFourScreenCircleMenuView, mCircleMenuParams);
			if(mFourScreenCircleMenuView != null){
				mFourScreenCircleMenuView.show();
			}
			removeViewHandler.sendEmptyMessageDelayed(send_hide_view_msg, CIRCLE_MENU_HIDE_DELAY);
		}
	}
	
	private final int CIRCLE_MENU_HIDE_DELAY = 3000;
	private void hideCircleMenu(){
		if(!ONE_LEVEL_MENU){
			return;
		}
		if(mFourScreenCircleMenuView != null){
			mFourScreenCircleMenuView.hide();
		}
		removeViewHandler.sendEmptyMessageDelayed(remove_view_msg, 600);
	}
	
	private final int remove_view_msg = 0x01;
	private final int send_hide_view_msg = 0x02;
	private Handler removeViewHandler = new Handler(){
		@Override
		public void handleMessage(Message msg) {
			if(!ONE_LEVEL_MENU){
				return;
			}
			switch (msg.what) {
			case send_hide_view_msg:
				hideCircleMenu();
				break;
			case remove_view_msg:
				if(mFourScreenCircleMenuView != null && mFourScreenCircleMenuView.getParent() != null){
					wm.removeView(mFourScreenCircleMenuView);
				}
				break;
			}
		}
	};
	
	private void addMultiModeWindow(){
		if(multiModeContainer == null){
			multiModeContainer = new LinearLayout(mContext);
			mMultiModeView = new ImageView(mContext);
			mMultiModeView.setLayoutParams(new LinearLayout.LayoutParams(68,68));
			setMultiModeView();
			mMultiModeView.setBackgroundResource(R.drawable.circle);
			mMultiModeView.setScaleType(ScaleType.CENTER);
			multiModeContainer.addView(mMultiModeView);
			multiModeContainer.setFocusable(true);
			multiModeContainer.setClickable(true);
			multiModeContainer.setOnTouchListener(new OnTouchListener() {		
				@Override
				public boolean onTouch(View v, MotionEvent event) {
						// TODO Auto-generated method stub
						changeMultiModeViewDrawable(event);
					mMultiModeDector.onTouchEvent(event);
							return true;
						}
					});
		}
		if(mMultiModeParams == null){
			mMultiModeParams = new WindowManager.LayoutParams();
			mMultiModeParams.type=WindowManager.LayoutParams.TYPE_MULTIMODE_BUTTON;//2002|WindowManager.LayoutParams.TYPE_SYSTEM_ALERT ;
			mMultiModeParams.format=PixelFormat.TRANSLUCENT;
			mMultiModeParams.flags|=8;
			mMultiModeParams.flags |=WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
			mMultiModeParams.width = WindowManager.LayoutParams.WRAP_CONTENT;
			mMultiModeParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
			mMultiModeParams.x = wm.getDefaultDisplay().getWidth();;
			mMultiModeParams.y = wm.getDefaultDisplay().getHeight()/2;
			mMultiModeParams.setTitle("MultiModeButton");
			mMultiModeParams.gravity=Gravity.LEFT|Gravity.TOP;

		}
		boolean show_config = false;
		if(show_config){
			multiModeContainer.setVisibility(View.VISIBLE);
		}else{
			multiModeContainer.setVisibility(View.GONE);
		}
		if(false){
			wm.addView(multiModeContainer, mMultiModeParams);
		}
	}
	
	private void addHalfScreenWindowController(){
		LOGD("addHalfScreenWindowController");
		if(mHSCParams == null){
			mHSCParams= new WindowManager.LayoutParams();
			mHSCParams.type=WindowManager.LayoutParams.TYPE_MULTIWINDOW_CONTROLLER;//2002|WindowManager.LayoutParams.TYPE_SYSTEM_ALERT ;
			mHSCParams.format=PixelFormat.TRANSLUCENT;
			mHSCParams.flags|=8;
			mHSCParams.flags |=WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
			mHSCParams.flags|= WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR;
			mHSCParams.flags|= WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
			
			//wmParams.flags |=WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD;
			//mHSCParams.windowAnimations = android.R.style.Animation_Translucent;
			mHSCParams.setTitle("MultiWidow/Controller");
			mHSCParams.gravity=Gravity.LEFT|Gravity.TOP;		 
			mHSCParams.width=-2;
			mHSCParams.height=-1;
			if(mContext.getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE){
				mHSCParams.x= mContext.getResources().getDisplayMetrics().widthPixels/2;
				mHSCParams.y= 0;
			}else if(mContext.getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT){
				mHSCParams.x = 0;
				mHSCParams.y = mContext.getResources().getDisplayMetrics().heightPixels/2;
			}
			updateSettingsSystem();
		}
		if(mHSContainer == null){
			loadDimens();
			mHSContainer = new LinearLayout(mContext);
			LinearLayout.LayoutParams llay = new LinearLayout.LayoutParams(-1, -1);
			//llay.setMargins(20, 0, 20, 0);
			mHSContainer.setLayoutParams(llay);
			mHSContainer.setPadding(MUL_CON_PAD, getStatusBarHeight(), MUL_CON_PAD, 0);
			mHSContainer.setOnTouchListener(new View.OnTouchListener() {				
				public boolean onTouch(View v, MotionEvent event) {
					// TODO Auto-generated method stub
					switch(event.getAction()){
						case MotionEvent.ACTION_UP:
							LOGD("action Up halfscreen controller");
							updateSettingsSystem();
							mHalfScreenController.setBackgroundResource(R.drawable.control_bg_normal);							
							break;
						case MotionEvent.ACTION_DOWN:
							LOGD("action down halfscreen contrllr");
							mHalfScreenController.setBackgroundResource(R.drawable.control_bg_pressed);							
							break;
						}
				mMoveDector.onTouchEvent(event);
					return true;
				}
			});
			mHalfScreenController = new ImageView(mContext);
			mHalfScreenController.setLayoutParams(new LinearLayout.LayoutParams(MUL_IMA_SIZE, -1));
			mHalfScreenController.setBackgroundResource(R.drawable.control_bg_normal);
			mHSContainer.addView(mHalfScreenController);
			
		}
		if(mMoveDector == null){
			mMoveDector = new GestureDetector(new MyGestureDetectorListener());
		}
		if(mMultiModeDector == null){
			mMultiModeDector = new GestureDetector(new MultiModeGestureDetectorListener());
		}
	}
	
	private void addFourScreenWindowController(){
		LOGD("addFourScreenWindowController");
		if(mFourScreenWCParams == null){
			mFourScreenWCParams= new WindowManager.LayoutParams();
			mFourScreenWCParams.type=WindowManager.LayoutParams.TYPE_MULTIWINDOW_CONTROLLER;//2002|WindowManager.LayoutParams.TYPE_SYSTEM_ALERT ;
			mFourScreenWCParams.format=PixelFormat.TRANSLUCENT;
			mFourScreenWCParams.flags|=8;
			mFourScreenWCParams.flags |=WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
			mFourScreenWCParams.flags|= WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR;
			mFourScreenWCParams.flags|= WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
			if(!FourScreenBackWindow.ONLY_ONE_BACK_WINDOW)
				mFourScreenWCParams.flags|=WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;
			
			//wmParams.flags |=WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD;
			//mHSCParams.windowAnimations = android.R.style.Animation_Translucent;
			mFourScreenWCParams.setTitle("MultiWidow/WController");
			mFourScreenWCParams.gravity=Gravity.LEFT|Gravity.TOP;	
			if(FourScreenBackWindow.ONLY_ONE_BACK_WINDOW){
				mFourScreenWCParams.width=-1;
			}else{
				mFourScreenWCParams.width=wm.getDefaultDisplay().getWidth();
			}
			mFourScreenWCParams.height=-2;
			mFourScreenWCParams.x = 0;
			mFourScreenWCParams.y = (mContext.getResources().getDisplayMetrics().heightPixels+mStatusBarHeight)/2-MUL_CON_POS;
			updateFourScreenSettingsSystem(LAND_CONTROL_TYPE);
		}
		if(mFSWSContainer == null){
			loadDimens();
			mFSWSContainer = new LinearLayout(mContext);
			LinearLayout.LayoutParams llay = new LinearLayout.LayoutParams(-1, -1);
			//llay.setMargins(20, 0, 20, 0);
			mFSWSContainer.setLayoutParams(llay);
			mFSWSContainer.setPadding(0, MUL_CON_PAD, 0, MUL_CON_PAD);
			mFSWSContainer.setOnTouchListener(new View.OnTouchListener() {				
				public boolean onTouch(View v, MotionEvent event) {
					// TODO Auto-generated method stub
					switch(event.getAction()){
						case MotionEvent.ACTION_CANCEL:
						case MotionEvent.ACTION_UP:
							LOGD("action Up fourscreen w controller");
							updateFourScreenSettingsSystem(LAND_CONTROL_TYPE);
							mFourScreenWController.setBackgroundResource(R.drawable.control_bg_normal);							
							break;
						/*case MotionEvent.ACTION_MOVE:
							LOGD("action MOVE fourscreen w controller");
							updateFourScreenSettingsSystem(LAND_CONTROL_TYPE);
							break;*/
						case MotionEvent.ACTION_DOWN:
							LOGD("action down fourscreen w contrllr");
							mFourScreenWController.setBackgroundResource(R.drawable.control_bg_pressed);							
							break;
						}
					mFourScreenWCMoveDector.onTouchEvent(event);
					return true;
				}
			});
			mFourScreenWController = new ImageView(mContext);
			mFourScreenWController.setLayoutParams(new LinearLayout.LayoutParams( -1 ,MUL_IMA_SIZE));
			mFourScreenWController.setBackgroundResource(R.drawable.control_bg_normal);
			mFSWSContainer.addView(mFourScreenWController);
			
		}
		if(mFourScreenWCMoveDector == null){
			mFourScreenWCMoveDector = new GestureDetector(new MyGestureOfFourScreenWDetectorListener());
		}
		
		if(mFourScreenHCParams == null){
			mFourScreenHCParams= new WindowManager.LayoutParams();
			mFourScreenHCParams.type=WindowManager.LayoutParams.TYPE_MULTIWINDOW_CONTROLLER;//2002|WindowManager.LayoutParams.TYPE_SYSTEM_ALERT ;
			mFourScreenHCParams.format=PixelFormat.TRANSLUCENT;
			mFourScreenHCParams.flags|=8;
			mFourScreenHCParams.flags |=WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
			mFourScreenHCParams.flags|= WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR;
			mFourScreenHCParams.flags|= WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
			if(!FourScreenBackWindow.ONLY_ONE_BACK_WINDOW)
				mFourScreenHCParams.flags|=WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;
			
			//wmParams.flags |=WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD;
			//mHSCParams.windowAnimations = android.R.style.Animation_Translucent;
			mFourScreenHCParams.setTitle("MultiWidow/HController");
			mFourScreenHCParams.gravity=Gravity.LEFT|Gravity.TOP;		 
			mFourScreenHCParams.width=-2;
			if(FourScreenBackWindow.ONLY_ONE_BACK_WINDOW){
				mFourScreenHCParams.height=-1;
			}else{
				mFourScreenHCParams.height=wm.getDefaultDisplay().getHeight();
			}
			mFourScreenHCParams.x = mContext.getResources().getDisplayMetrics().widthPixels/2-MUL_CON_POS;
			mFourScreenHCParams.y = mStatusBarHeight;
			updateFourScreenSettingsSystem(PORT_CONTROL_TYPE);
		}
		if(mFSHSContainer == null){
			loadDimens();
			mFSHSContainer = new LinearLayout(mContext);
			LinearLayout.LayoutParams llay = new LinearLayout.LayoutParams(-1, -1);
			//llay.setMargins(20, 0, 20, 0);
			mFSHSContainer.setLayoutParams(llay);
			mFSHSContainer.setPadding(MUL_CON_PAD, getStatusBarHeight(), MUL_CON_PAD, 0);
			mFSHSContainer.setOnTouchListener(new View.OnTouchListener() {				
				public boolean onTouch(View v, MotionEvent event) {
					// TODO Auto-generated method stub
					switch(event.getAction()){
						case MotionEvent.ACTION_CANCEL:
						case MotionEvent.ACTION_UP:
							LOGD("action Up fourscreen h controller");
							updateFourScreenSettingsSystem(PORT_CONTROL_TYPE);
							mFourScreenHController.setBackgroundResource(R.drawable.control_bg_normal);							
							break;
						/*case MotionEvent.ACTION_MOVE:
							LOGD("action MOVE fourscreen h controller");
							updateFourScreenSettingsSystem(PORT_CONTROL_TYPE);
							break;*/
						case MotionEvent.ACTION_DOWN:
							LOGD("action down fourscreen h contrllr");
							mFourScreenHController.setBackgroundResource(R.drawable.control_bg_pressed);							
							break;
						}
					mFourScreenHCMoveDector.onTouchEvent(event);
					return true;
				}
			});
			mFourScreenHController = new ImageView(mContext);
			mFourScreenHController.setLayoutParams(new LinearLayout.LayoutParams(MUL_IMA_SIZE, -1));
			mFourScreenHController.setBackgroundResource(R.drawable.control_bg_normal);
			mFSHSContainer.addView(mFourScreenHController);
			
		}
		if(mFourScreenHCMoveDector == null){
			mFourScreenHCMoveDector = new GestureDetector(new MyGestureOfFourScreenHDetectorListener());
		}
	}

    //huangjc win bar
    private ContentObserver mMultiConfigObserver = new ContentObserver(new Handler()){
		     @Override
		     public void onChange(boolean selfChange) {
			            final boolean isshow = 0 != Settings.System.getInt(
			                    mContext.getContentResolver(), Settings.System.MULTI_WINDOW_CONFIG, 0);
			                    isMultiChange=true;
					/*	       try {
					            IActivityManager am = ActivityManagerNative.getDefault();
					            Configuration config = am.getConfiguration();
					
					            // Will set userSetLocale to indicate this isn't some passing default - the user
					            // wants this remembered
					            config.setMultiWindowFlag(isshow);
					
					            am.updateConfiguration(config);
					            // Trigger the dirty bit for the Settings Provider.
					            //BackupManager.dataChanged("com.android.providers.settings");
					        } catch (RemoteException e) {
					            // Intentionally left blank
					        }
		            */
									if(isshow){
										LOGD("=======MULTI_WINDOW_CONFIG is open ========");
										if(appslist!=null && appslist.size() > 0)
										 appslist.clear();
									}else{
										LOGD("=======MULTI_WINDOW_CONFIG is close ========");
									}
						
					}

	};
private String lastInputMethodId = null;
	private ContentObserver mExtkeyboardObserver = new ContentObserver(new Handler()){
		     @Override
		     public void onChange(boolean selfChange) {
		     if(!mContext.getResources().getConfiguration().enableMultiWindow())
			 	return;
	            final boolean isshow = 0 != Settings.System.getInt(
	                   mContext.getContentResolver(), Settings.System.EXTER_KEYBOARD_CONFIG, 0);
	                  InputMethodManager imm = (InputMethodManager) mContext.getSystemService(Context.INPUT_METHOD_SERVICE);
					  String InputMethodId = Settings.Secure.getStringForUser(
                                                       mContext.getContentResolver(), Settings.Secure.DEFAULT_INPUT_METHOD,0);
					  if(!"com.google.android.inputmethod.pinyin/.PinyinIME".equals(InputMethodId))
					  lastInputMethodId = InputMethodId;
					  
					   List<InputMethodInfo> infos = imm.getInputMethodList();
					    String id = null;
					    for (InputMethodInfo info : infos) {
					        if ("com.google.android.inputmethod.pinyin/.PinyinIME".equals(info.getId())){
					            id = info.getId();
					        }
					    }
					    if (id == null) {
					        return;
					    }
					if(isshow){
						LOGD("=======EXTER_KEYBOARD_CONFIG is open ========id:"+id);
						  imm.setInputMethod(null,id);
					}else{
						LOGD("=======EXTER_KEYBOARD_CONFIG is close ========lastInputMethodId"+lastInputMethodId);
						if(lastInputMethodId!=null&&!lastInputMethodId.equals("0"))
							imm.setInputMethod(null,lastInputMethodId);
					}
				
			}

	};
	
	private void updateFourScreenControllers(String str){
		if(mFourScreenHCParams == null || mFourScreenWCParams == null){
			return;
		}
		if (!FourScreenBackWindow.ONLY_ONE_BACK_WINDOW) {
			String areas = str;
			if (areas == null) {
				return;
			}
			int x = mFourScreenHCParams.x + MUL_CON_POS;
			int y = mFourScreenWCParams.y + MUL_CON_POS;
			LOGD("------------updateFourScreenControllers----------------areas:"+areas+"x:"+x+",y:"+y);
			int SCREEN_WIDTH = wm.getDefaultDisplay().getWidth();
			int SCREEN_HEIGHT = wm.getDefaultDisplay().getHeight();
			int mWControllerX = 0;
			if (areas.indexOf("0") == -1 && areas.indexOf("2") == -1) {
				mWControllerX = x;
			}
			if (areas.indexOf("1") == -1 && areas.indexOf("3") == -1) {
				if(mWControllerX == x){
					mWControllerX = wm.getDefaultDisplay().getWidth();
				}else{
					mWControllerX = x - wm.getDefaultDisplay().getWidth();
				}
			}
			if (mFourScreenWCParams != null) {
				mFourScreenWCParams.x = mWControllerX;
			}

			int mHControllerY = 0;
			if (areas.indexOf("0") == -1 && areas.indexOf("1") == -1) {
				mHControllerY = y - mStatusBarHeight;
			}
			if (areas.indexOf("2") == -1 && areas.indexOf("3") == -1) {
				if(mHControllerY == y - mStatusBarHeight){
					mHControllerY = wm.getDefaultDisplay().getHeight();
				}else{
					mHControllerY = y-wm.getDefaultDisplay().getHeight();
				}
				
			}
			if (mFourScreenHCParams != null) {
				mFourScreenHCParams.y = mHControllerY;
			}
		}
		if(IS_USE_BACK_WINDOW){
			if(mFourScreenBackWindow != null){
				mFourScreenBackWindow.updateBackWindowView(mFourScreenHCParams.x+MUL_CON_POS,mFourScreenWCParams.y+MUL_CON_POS);
			}
		}
		//updateFourScreenSettingsSystem(LAND_CONTROL_TYPE);
		//updateFourScreenSettingsSystem(PORT_CONTROL_TYPE);
		if(mFSWSContainer != null && mFSWSContainer.getParent() != null){
			wm.updateViewLayout(mFSWSContainer, mFourScreenWCParams);
		}
		if(mFSHSContainer != null && mFSHSContainer.getParent() != null){
			wm.updateViewLayout(mFSHSContainer, mFourScreenHCParams);
		}
	}
	
    private void updateHSCParams(){
		String pos = Settings.System.getString(mContext.getContentResolver(),
											Settings.System.HALF_SCREEN_WINDOW_POSITION);
		LOGD("updateHSCParams position="+pos);
		int x=0;
		int y=0;
		if(pos !=null){
			String[] position = pos.split(",");
			x = Integer.parseInt(position[0]);
			y = Integer.parseInt(position[1]);
		}
		if(x == 0){
			y-=MUL_CON_POS;
		}
		if(y == 0){
			x-=MUL_CON_POS;
		}
		mHSCParams.x = x;
		mHSCParams.y = y;
	}
	
	private void updateSettingsSystem(){
		int x = mHSCParams.x;
		int y = mHSCParams.y;
		if(x ==0){
			y+=MUL_CON_POS;
		}
		if(y == 0){
			x+=MUL_CON_POS;
		}
		String pos = ""+x+","+y;
		LOGD(""+pos);
		Settings.System.putString(mContext.getContentResolver(),
								Settings.System.HALF_SCREEN_WINDOW_POSITION, pos);
	}
	
	private void updateFourScreenSCParams(int type){
		String pos = Settings.System.getString(mContext.getContentResolver(),
											Settings.System.FOUR_SCREEN_WINDOW_POSITION);
		LOGD("updateFourScreenSCParams position="+pos);
		int x=0;
		int y=0;
		if(type == LAND_CONTROL_TYPE){
			if(pos !=null){
				String[] position = pos.split(",");
				y = Integer.parseInt(position[1]);
			}
			//y-=MUL_CON_POS;
			if(mFourScreenWCParams != null){
				mFourScreenWCParams.x = x;
				mFourScreenWCParams.y = y;
			}
			if(mCenterBtnParams != null){
				mCenterBtnParams.y = y  - CENTER_BTN_HEIGHT/2;
			}
		}else if(type == PORT_CONTROL_TYPE){
			if(pos !=null){
				String[] position = pos.split(",");
				x = Integer.parseInt(position[0]);
			}
			//x-=MUL_CON_POS;
			if(mFourScreenHCParams != null){
				mFourScreenHCParams.x = x;
				mFourScreenHCParams.y = y;
			}
			if(mCenterBtnParams != null){
				mCenterBtnParams.x = x  - CENTER_BTN_WIDTH/2;
			}
		}
	}
	
	private int LAND_CONTROL_TYPE = 0x01;
	private int PORT_CONTROL_TYPE = 0x02;
	private void updateFourScreenSettingsSystem(int type){
		String pos = Settings.System.getString(mContext.getContentResolver(),
				Settings.System.FOUR_SCREEN_WINDOW_POSITION);
		int x=0;
		int y=0;
		if(pos !=null){
			String[] position = pos.split(",");
			x = Integer.parseInt(position[0]);
			y = Integer.parseInt(position[1]);
		}
		if(type == LAND_CONTROL_TYPE && mFourScreenWCParams != null){
			y = mFourScreenWCParams.y;
			y+=MUL_CON_POS;
		}else if(type == PORT_CONTROL_TYPE && mFourScreenHCParams != null){
			x = mFourScreenHCParams.x;
			x+=MUL_CON_POS;
		}else{
			y = mCenterBtnParams.y;
			y+=CENTER_BTN_WIDTH/2;
			x = mCenterBtnParams.x;
			x+=CENTER_BTN_HEIGHT/2;
		}
		String savePos = ""+x+","+y;
		LOGD("----------------"+savePos);
		Settings.System.putString(mContext.getContentResolver(),
								Settings.System.FOUR_SCREEN_WINDOW_POSITION, savePos);
		if(IS_USE_BACK_WINDOW){
			if(mFourScreenBackWindow != null){
				mFourScreenBackWindow.updateBackWindowView();
				mFourScreenBackWindow.updateAddButtonWindowView();
			}
		}
	}
	
	@Override
    protected void onConfigurationChanged(Configuration newConfig) {
    	super.onConfigurationChanged(newConfig); // calls refreshLayout

        if (DEBUG) {
            Log.v(TAG, "configuration changed: " + mContext.getResources().getConfiguration());
        }
        
        //huangjc win bar
         if(isMultiChange){
		 	if("box".equals(SystemProperties.get("ro.target.product", "tablet"))){
				isMultiChange = false;
	//	 	 android.os.Process.killProcess(android.os.Process.myPid()); 
			 return;
		 		}
	        removeBar();
					try {
			    Thread.sleep(1500);
					} catch (Exception ex) {
						 //Log.e(TAG,"Exception:" +ex.getMessage());
					}
		      addBar();
		      isMultiChange = false;
		  }
		    	
        updateDisplaySize(); // populates mDisplayMetrics

        updateResources();
        repositionNavigationBar();
        updateExpandedViewPos(EXPANDED_LEAVE_ALONE);
        updateShowSearchHoldoff();

        WindowManager.LayoutParams lp = (WindowManager.LayoutParams) mStatusBarWindow.getLayoutParams();
        if (lp != null) {
            mWindowManager.updateViewLayout(mStatusBarWindow, lp);
        }
		
        loadDimens();
		
		
        mShowSearchHoldoff = mContext.getResources().getInteger(
                R.integer.config_show_search_delay);
        updateSearchPanel();	   
        if(newConfig.enableMultiWindow()){
        	
//        	if(getMultiMode() == Settings.System.MULITI_WINDOW_HALF_SCREEN_MODE){
        		if(newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE){
        			mHSCParams.x = wm.getDefaultDisplay().getWidth()/2;
        			mHSCParams.y = 0;
        			mHSCParams.width = -2;
        			mHSCParams.height = ViewGroup.LayoutParams.MATCH_PARENT;
        			if(mHSContainer!=null){
        				LinearLayout.LayoutParams llay = new LinearLayout.LayoutParams(-1, -1);
        				mHSContainer.setLayoutParams(llay);
        				mHSContainer.setPadding(MUL_CON_PAD, getStatusBarHeight(), MUL_CON_PAD, 0);
        			}
        			if(mHalfScreenController!=null){
        				mHalfScreenController.setLayoutParams(new LinearLayout.LayoutParams(MUL_IMA_SIZE, -1));
        			}
        		}else if(newConfig.orientation == Configuration.ORIENTATION_PORTRAIT){
        			mHSCParams.y = wm.getDefaultDisplay().getHeight()/2;
        			mHSCParams.x = 0;
        			mHSCParams.height = -2;
        			mHSCParams.width = ViewGroup.LayoutParams.MATCH_PARENT;
        			if(mHSContainer!=null){
        				LinearLayout.LayoutParams llay = new LinearLayout.LayoutParams(-1, -1);
        				mHSContainer.setLayoutParams(llay);
        				mHSContainer.setPadding(0, MUL_CON_PAD, 0, MUL_CON_PAD);
        			}
        			if(mHalfScreenController!=null){
        				mHalfScreenController.setLayoutParams(new LinearLayout.LayoutParams(-1, MUL_IMA_SIZE));
        			}
        		}
        		if(mHSContainer.getParent()!=null){
        			wm.updateViewLayout(mHSContainer,mHSCParams);
        		}
//        	}else if(getMultiMode() == Settings.System.MULITI_WINDOW_FOUR_SCREEN_MODE){
        		if(mFourScreenWCParams != null){
        			mFourScreenWCParams.x = 0;
        			mFourScreenWCParams.y = (wm.getDefaultDisplay().getHeight()+mStatusBarHeight)/2-MUL_CON_POS;
        			if(FourScreenBackWindow.ONLY_ONE_BACK_WINDOW){
        				mFourScreenWCParams.width = -1;
        			}else{
        				mFourScreenWCParams.width = wm.getDefaultDisplay().getWidth();
        			}
        			mFourScreenWCParams.height = -2;
        			if(mFSWSContainer!=null){
        				LinearLayout.LayoutParams llay = new LinearLayout.LayoutParams(-1, -1);
        				mFSWSContainer.setLayoutParams(llay);
        				mFSWSContainer.setPadding(0, MUL_CON_PAD, 0, MUL_CON_PAD);
        			}
        			if(mFourScreenWController!=null){
        				mFourScreenWController.setLayoutParams(new LinearLayout.LayoutParams(-1,MUL_IMA_SIZE));
        			}
        			if(mFSWSContainer.getParent()!=null){
        				wm.updateViewLayout(mFSWSContainer,mFourScreenWCParams);
        			}
        		
        			mFourScreenHCParams.x = wm.getDefaultDisplay().getWidth()/2-MUL_CON_POS;
        			mFourScreenHCParams.y = 0;
        			if(FourScreenBackWindow.ONLY_ONE_BACK_WINDOW){
        				mFourScreenHCParams.height = -1;
        			}else{
        				mFourScreenHCParams.height = wm.getDefaultDisplay().getHeight();
        			}
        			mFourScreenHCParams.width = -2;
        			if(mFSHSContainer!=null){
        				LinearLayout.LayoutParams llay = new LinearLayout.LayoutParams(-1, -1);
        				mFSHSContainer.setLayoutParams(llay);
        				mFSHSContainer.setPadding(MUL_CON_PAD, getStatusBarHeight(), MUL_CON_PAD, 0);
        			}
        			if(mFourScreenHController!=null){
        				mFourScreenHController.setLayoutParams(new LinearLayout.LayoutParams(MUL_IMA_SIZE,-1));
        			}
        			if(mFSHSContainer.getParent()!=null){
        				wm.updateViewLayout(mFSHSContainer,mFourScreenHCParams);
        			}
        			updateCenterBtnView();
        		}else{
        			mCenterBtnParams.x = wm.getDefaultDisplay().getWidth()/2 - CENTER_BTN_WIDTH/2;
        			mCenterBtnParams.y = (wm.getDefaultDisplay().getHeight()+mStatusBarHeight)/2 - CENTER_BTN_HEIGHT/2;
        			if (wm != null && mCenterBtnContainer.getParent() != null) {
        				wm.updateViewLayout(mCenterBtnContainer, mCenterBtnParams);
        			}
        		}
//        	}
        }
		mMultiModeParams.x = wm.getDefaultDisplay().getWidth()-(multiModeContainer.getWidth()/2);
		mMultiModeParams.y = wm.getDefaultDisplay().getHeight()/2-multiModeContainer.getHeight();
		if(multiModeContainer.getParent()!=null){
			setMultiModeView();
			wm.updateViewLayout(multiModeContainer,mMultiModeParams);
			if(newConfig.enableMultiWindow()){
				boolean show_config = false;
				if(show_config){
					multiModeContainer.setVisibility(View.VISIBLE);
				}else{
					multiModeContainer.setVisibility(View.GONE);
				}
			}else{
				multiModeContainer.setVisibility(View.GONE);
			}
		}
		LOGR("configuration change newConfig="+newConfig);
		
		if(mAppBarPanel != null){
			if(newConfig.enableMultiWindow()){
				LOGR("enable the panelView");
				mAppBarPanel.setPanelViewEnabled(false);
			}else{
				LOGR("disable the panelView");
				mAppBarPanel.setPanelViewEnabled(false);
			}
		}
		
		updateSettingsSystem();
		if((getMultiMode() == Settings.System.MULITI_WINDOW_FOUR_SCREEN_MODE)){
			LOGD("---------------onConfigurationChanged---------------------");
			updateFourScreenSettingsSystem(LAND_CONTROL_TYPE);
			updateFourScreenSettingsSystem(PORT_CONTROL_TYPE);	
		}
		updateFourScreenControllers(null);
		updateCircleMenu();
    }

	private int clickWindowArea = -1;
	@Override
	public void onClick(View v) {
		Settings.System.putString(mContext.getContentResolver(),
				Settings.System.MULTI_WINDOW_OPERATION, "");
		updateFourScreenControllers(null);
		int index = (Integer)v.getTag();
		int area = areaMap.get(index/3);
		int operation = FourScreenCircleMenuView.btnOperationMap.get(index % FourScreenCircleMenuView.MENU_COUNT_PER_AREA);
		clickWindowArea = area;
		if(operation == Settings.System.MULTI_WINDOW_MAX){
			LOGD("-----------onClick MULTI_WINDOW_MAX------------");
			if( mFSWSContainer != null && mFSWSContainer.getParent()!=null){
				wm.removeView(mFSWSContainer);
			}
			if( mFSHSContainer != null && mFSHSContainer.getParent()!=null){
				wm.removeView(mFSHSContainer);
			}
			if( mCenterBtnContainer != null && mCenterBtnContainer.getParent()!=null){
				wm.removeView(mCenterBtnContainer);
			}
		}
		if(ONE_LEVEL_MENU){
			if( mFourScreenCircleMenuView.getParent()!=null){
				wm.removeView(mFourScreenCircleMenuView);
			}
		}
		String clickAction = ""+area+","+operation;
		LOGD(index+"   clickAction:"+clickAction);
		Settings.System.putString(mContext.getContentResolver(),
				Settings.System.MULTI_WINDOW_OPERATION, clickAction);
	}
  


}
