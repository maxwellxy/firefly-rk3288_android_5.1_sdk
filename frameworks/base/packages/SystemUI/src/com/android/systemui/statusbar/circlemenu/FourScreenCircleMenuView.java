package com.android.systemui.statusbar.circlemenu;

import java.util.ArrayList;
import java.util.HashMap;

import com.android.systemui.R;
import com.android.systemui.statusbar.circlemenu.CircleMenuView.BtnClickCallBack;
import com.android.systemui.statusbar.phone.PhoneStatusBar;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.FrameLayout.LayoutParams;
import android.provider.Settings;

public class FourScreenCircleMenuView extends FrameLayout {
	
	private Context mContext = null;
	private LayoutInflater mLayoutInflater;
	
	public static int WIDTH = 300;
	
	public static int HEIGHT = 300;
	
	public static int ITEM_WIDTH = 52;
	
	public static int ITEM_HEIGHT = 52;
	
	private CircleMenuView mCircleMenuView = null;
	
	public static final HashMap<Integer, Integer> btnOperationMap = new HashMap<Integer, Integer>();
	static {
		btnOperationMap.put(0, Settings.System.MULTI_WINDOW_CLOSE);
		btnOperationMap.put(1, Settings.System.MULTI_WINDOW_MIN);
		btnOperationMap.put(2, Settings.System.MULTI_WINDOW_MAX);
	}
	
	public static final int MENU_COUNT_PER_AREA = 3;
	
	public static int CIRCLE_RADIUS = 17;

	public FourScreenCircleMenuView(Context context, AttributeSet attrs,
			int defStyle) {
		super(context, attrs, defStyle);
		mContext = context;
		setupView();
		System.out.println("============================================================FourScreenCircleMenuView");
	}

	public FourScreenCircleMenuView(Context context, AttributeSet attrs) {
		super(context, attrs);
		mContext = context;
		setupView();
		System.out.println("============================================================FourScreenCircleMenuView");
	}

	public FourScreenCircleMenuView(Context context) {
		super(context);
		mContext = context;
		setupView();
		System.out.println("============================================================FourScreenCircleMenuView");
	}

	private void setupView(){
		WIDTH = (int)mContext.getResources().getDimension(R.dimen.four_screen_center_panel_width);
		HEIGHT = (int)mContext.getResources().getDimension(R.dimen.four_screen_center_panel_height);
		ITEM_WIDTH = (int)mContext.getResources().getDimension(R.dimen.center_btn_width);
		ITEM_HEIGHT = (int)mContext.getResources().getDimension(R.dimen.center_btn_height);
		CIRCLE_RADIUS = (int)mContext.getResources().getDimension(R.dimen.center_menu_circle_radius);
		mLayoutInflater = (LayoutInflater) mContext
				.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		View view = mLayoutInflater.inflate(
				R.layout.circle_menu, null);
		LayoutParams lp = new LayoutParams(WIDTH,HEIGHT);
		this.addView(view, lp);
		mCircleMenuView = (CircleMenuView)view.findViewById(R.id.circle_menu);
	}
	
	public void show(){
		if(mCircleMenuView != null){
			mCircleMenuView.startAnimationOut();
		}
	}
	
	public void hide(){
		if(mCircleMenuView != null){
			mCircleMenuView.startAnimationIn();
		}
	}
	
	public void showAreasMenu(String areas){
		ArrayList<Integer> list = new ArrayList<Integer>();
		if(areas != null && areas.indexOf(",") > 0){
			String steps[] = areas.split(",");
			for(int i = 0;i < steps.length;i++){
				if(steps[i] != null){
					String str = steps[i];
					if(str.equals("0") || str.equals("1") || str.equals("2") || str.equals("3")){
						list.add(PhoneStatusBar.areaMap.get(Integer.valueOf(str)));
					}
				}
			}
		}
		if(mCircleMenuView != null){
			mCircleMenuView.setAreasMenuVisiable(list);
		}
	}
	
	public void setBtnClickCallBack(BtnClickCallBack b){
		if(mCircleMenuView != null){
			mCircleMenuView.setBtnClickCallBack(b);
		}
	}
}
