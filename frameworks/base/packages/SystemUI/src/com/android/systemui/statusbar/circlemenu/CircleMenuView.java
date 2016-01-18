package com.android.systemui.statusbar.circlemenu;

import java.util.ArrayList;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.AnimationSet;
import android.view.animation.AnticipateInterpolator;
import android.view.animation.OvershootInterpolator;
import android.view.animation.TranslateAnimation;
import android.widget.FrameLayout;
import android.widget.ImageView;
import com.android.systemui.R;

public class CircleMenuView extends FrameLayout implements View.OnClickListener {

	private ImageView mHome;
	private Context mContext;
	private float mDensity;
	private boolean bMenuShow = true;

	private static int xOffset = 15;
	private static int yOffset = -13;
	private int[] menuResIds = { R.drawable.circlemenu_close, R.drawable.circlemenu_narrow,
			R.drawable.circlemenu_larger };

	private BtnClickCallBack mBtnClickCallBack;

	public CircleMenuView(Context context) {
		super(context);
		setupViews();
	}

	public CircleMenuView(Context context, AttributeSet attrs) {
		super(context, attrs);
		setupViews();
	}

	private void setupViews() {
		mContext = getContext();
		mDensity = mContext.getResources().getDisplayMetrics().density;

		xOffset = (int) (10.667 * mDensity);
		yOffset = (int) (8.667 * mDensity);

		mHome = new ImageView(mContext);

		// mHome.setImageResource(R.drawable.composer_button);

		addView(mHome);

		LayoutParams mHomeparams = (FrameLayout.LayoutParams) mHome
				.getLayoutParams();
		mHomeparams.width = 32;
		mHomeparams.height = 32;
		int size = menuResIds.length * 4;
		int r = (int) (2 * FourScreenCircleMenuView.CIRCLE_RADIUS * size / Math.PI);
		int lenght = size - 1;
		mHomeparams.gravity = Gravity.CENTER;

		for (int i = 0; i < size; i++) {
			int x = (int) (Math.cos(2 * Math.PI * (i + size / 2 + 0.5)
					/ (lenght + 1)) * r);
			int y = -(int) (Math.sin(2 * Math.PI * (i + size / 2 + 0.5)
					/ (lenght + 1)) * r);
			ImageView imageView = new ImageView(mContext);
			imageView.setImageResource(menuResIds[i
					% FourScreenCircleMenuView.MENU_COUNT_PER_AREA]);
			imageView.setVisibility(View.GONE);
			addView(imageView);
			LayoutParams params = (FrameLayout.LayoutParams) imageView
					.getLayoutParams();
			params.width = FourScreenCircleMenuView.ITEM_WIDTH;
			params.height = FourScreenCircleMenuView.ITEM_HEIGHT;
			params.gravity = Gravity.CENTER;
			params.leftMargin = x;
			params.bottomMargin = y;
			imageView.setLayoutParams(params);
			imageView.setTag(i);
			imageView.setOnClickListener(this);
		}
		mHome.setLayoutParams(mHomeparams);
		startAnimationIn();
	}

	public void setBtnClickCallBack(BtnClickCallBack btnClickCallBack) {
		mBtnClickCallBack = btnClickCallBack;
	}

	public void startAnimationIn() {
		if (bMenuShow) {
			bMenuShow = false;
			startAnimationIn(CircleMenuView.this, 300);
		}
	}

	public void startAnimationOut() {
		if (!bMenuShow) {
			bMenuShow = true;
			startAnimationOut(CircleMenuView.this, 300);
		}
	}

	public void setAreasMenuVisiable(ArrayList<Integer> list) {
		ViewGroup group = CircleMenuView.this;
		for (int i = 1; i < group.getChildCount(); i++) {
			ImageView imageview = (ImageView) group.getChildAt(i);
			int step = (i - 1) / FourScreenCircleMenuView.MENU_COUNT_PER_AREA;
			if (list != null && list.contains(step)) {
				if(imageview.getVisibility() != View.VISIBLE){
					startAnimationOutOfImageView(imageview,300,i);
				}
			} else {
				imageview.setAnimation(null);
				imageview.setVisibility(View.GONE);
			}
		}
	}

	private void startAnimationOutOfImageView(ImageView imageview,
			int duration, int step) {
		imageview.setVisibility(View.VISIBLE);
		MarginLayoutParams mlp = (MarginLayoutParams) imageview
				.getLayoutParams();
		// 动画集
		AnimationSet set = new AnimationSet(true);
		Animation animation = null;
		animation = new TranslateAnimation(-mlp.leftMargin, 0f,
				mlp.bottomMargin, 0f);
		animation.setFillAfter(true);
		animation.setStartOffset(step * 50);
		animation.setInterpolator(new AnticipateInterpolator(2F));
		// 初始化 Alpha动画
		AlphaAnimation alphaAnimation = new AlphaAnimation(0.1f, 1.0f);
		set.addAnimation(animation);
		set.addAnimation(alphaAnimation);
		set.setDuration(duration);
		imageview.startAnimation(set);
	}

	private void startAnimationIn(ViewGroup group, int duration) {
		int step = 0;
		for (int i = 1; i < group.getChildCount(); i++) {
			final ImageView imageview = (ImageView) group.getChildAt(i);
			if (imageview.getVisibility() != View.VISIBLE) {
				imageview.setVisibility(View.GONE);
				imageview.setAnimation(null);
				continue;
			}
			step += 1;
			MarginLayoutParams mlp = (MarginLayoutParams) imageview
					.getLayoutParams();
			// 动画集
			AnimationSet set = new AnimationSet(true);
			Animation animation = null;
			animation = new TranslateAnimation(0f, -mlp.leftMargin, 0f,
					mlp.bottomMargin);

			animation.setFillAfter(true);
			animation.setStartOffset(step * 50);
			animation.setInterpolator(new OvershootInterpolator(2F));
			// 初始化 Alpha动画
			AlphaAnimation alphaAnimation = new AlphaAnimation(1.0f, 0.1f);
			set.addAnimation(animation);
			set.addAnimation(alphaAnimation);
			set.setDuration(duration);
			set.setAnimationListener(new Animation.AnimationListener() {
				@Override
				public void onAnimationStart(Animation animation) {
				}

				@Override
				public void onAnimationRepeat(Animation animation) {
				}

				@Override
				public void onAnimationEnd(Animation animation) {
					imageview.setVisibility(View.INVISIBLE);
				}
			});
			imageview.startAnimation(set);

		}
	}

	private void startAnimationOut(ViewGroup group, int duration) {
		int step = 0;
		for (int i = 1; i < group.getChildCount(); i++) {
			final ImageView imageview = (ImageView) group.getChildAt(i);
			if (imageview.getVisibility() == View.GONE) {
				imageview.setAnimation(null);
				continue;
			} else {
				imageview.setVisibility(View.VISIBLE);
			}
			step += 1;
			MarginLayoutParams mlp = (MarginLayoutParams) imageview
					.getLayoutParams();
			// 动画集
			AnimationSet set = new AnimationSet(true);
			Animation animation = null;
			animation = new TranslateAnimation(-mlp.leftMargin, 0f,
					mlp.bottomMargin, 0f);
			animation.setFillAfter(true);
			animation.setStartOffset(step * 50);
			animation.setInterpolator(new AnticipateInterpolator(2F));
			// 初始化 Alpha动画
			AlphaAnimation alphaAnimation = new AlphaAnimation(0.1f, 1.0f);
			set.addAnimation(animation);
			set.addAnimation(alphaAnimation);
			set.setDuration(duration);
			imageview.startAnimation(set);
		}
	}

	@Override
	public void onClick(View v) {
		Log.v("zjy", "-------onClick----------v.getTag():" + v.getTag());
		if (mBtnClickCallBack != null) {
			mBtnClickCallBack.onClick(v);
		}
	}

	public interface BtnClickCallBack {
		public void onClick(View v);
	};
}
