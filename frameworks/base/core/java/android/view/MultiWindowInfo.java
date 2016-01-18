package android.view;

import android.content.res.CompatibilityInfo.Translator;
import android.os.Parcelable;
import android.os.Parcel;
import android.graphics.Rect;
import android.util.Log;

/**
*@hide
*/
public  class MultiWindowInfo implements Parcelable{
	private static final String TAG = "MultiWindowInfo";
	private static final boolean DEBUG_FJZ = false;
	private void LOGD(String msg){
		if(DEBUG_FJZ){
			Log.d(TAG,msg);
		}
	}
		public int mPosX;
		public int mPosY;
		public int offsetX;
		public int offsetY;
		public int mCurrentWidth;
		public int mCurrentHeight;
		public float mHScale;
		public float mVScale;
		public float mActualScale;
		public int mRotation;
		public int mFocused;
		public Rect mVisibleFrame = new Rect();
		public Rect mSystemDecorRect = new Rect();
		public Rect mDecorFrame =new Rect();
		public static int FLAG_FOCUSED = 1;
		public static int FLAG_NOT_FOCUSED = 0;
		
		 public MultiWindowInfo(){
			mPosX = 0;
			mPosY = 0;
			offsetX = 0;
			offsetY = 0;
			mCurrentWidth = 0;
			mCurrentHeight = 0;
			mHScale = 1.0f;
			mVScale = 1.0f;
			mActualScale = 1.0f;
			mRotation = 0;
			mFocused = FLAG_NOT_FOCUSED;
			mVisibleFrame.setEmpty();
			mSystemDecorRect.setEmpty();	
			mDecorFrame.setEmpty();
		 }
		 public MultiWindowInfo(Parcel in){
			mPosX = in.readInt();
			mPosY = in.readInt();
			offsetX = in.readInt();
			offsetY = in.readInt(); 
			mCurrentWidth = in.readInt();
			mCurrentHeight = in.readInt();
			mHScale = in.readFloat();
			mVScale = in.readFloat();
			mActualScale = in.readFloat();
			mRotation = in.readInt();
			mFocused = in.readInt();
			mVisibleFrame.readFromParcel(in);
			mSystemDecorRect.readFromParcel(in);
			mDecorFrame.readFromParcel( in);
		 }

		 public void set(MultiWindowInfo info){
			mPosX = info.mPosX;
			mPosY = info.mPosY;
			offsetX = info.offsetX;
			offsetY = info.offsetY;
			mCurrentWidth = info.mCurrentWidth;
			mCurrentHeight = info.mCurrentHeight;
			mHScale = info.mHScale;
			mVScale = info.mVScale;
			mActualScale = info.mActualScale;
			mRotation = info.mRotation;
			mFocused = info.mFocused;
			mVisibleFrame = info.mVisibleFrame;
			mSystemDecorRect = info.mSystemDecorRect;	
			mDecorFrame = info.mDecorFrame;
		 }
		 
	@Override
   	 public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        MultiWindowInfo r = (MultiWindowInfo) o;
        return mPosX == r.mPosX&& mPosY == r.mPosY && mCurrentWidth== r.mCurrentWidth
				&& mCurrentHeight == r.mCurrentHeight && mHScale == r.mHScale && 
				mVScale == r.mVScale && mActualScale == r.mActualScale && 
				mRotation == r.mRotation && mVisibleFrame == r.mVisibleFrame &&
				offsetX == r.offsetX&& offsetY == r.offsetY && mSystemDecorRect == r.mSystemDecorRect 
				&& mFocused == r.mFocused && mDecorFrame == r.mDecorFrame  ;
    }
		public int describeContents() {
			// TODO Auto-generated method stub
			return 0;
		}
		public void writeToParcel(Parcel dest, int flags) {
			// TODO Auto-generated method stub
			dest.writeInt(mPosX);
			dest.writeInt(mPosY);
			dest.writeInt(offsetX);
			dest.writeInt(offsetY);
			dest.writeInt(mCurrentWidth);
			dest.writeInt(mCurrentHeight);
			dest.writeFloat(mHScale);
			dest.writeFloat(mVScale);
			dest.writeFloat(mActualScale);
			dest.writeInt(mRotation);
			dest.writeInt(mFocused);
			mVisibleFrame.writeToParcel(dest,flags);
			mSystemDecorRect.writeToParcel(dest,flags);		
			mDecorFrame.writeToParcel(dest,flags);
		}
		
	    public static final Parcelable.Creator<MultiWindowInfo> CREATOR = new Parcelable.Creator<MultiWindowInfo>() {
	        /**
	         * Return a new rectangle from the data in the specified parcel.
	         */
	        public MultiWindowInfo createFromParcel(Parcel in) {
	           // MultiWindowInfo r = 
	            //r.readFromParcel(in);
	            return new MultiWindowInfo(in);
	        }

	        /**
	         * Return an array of rectangles of the specified size.
	         */
	        public MultiWindowInfo[] newArray(int size) {
	            return new MultiWindowInfo[size];
	        }
	    };
	    public void readFromParcel(Parcel in) {
	        mPosX = in.readInt();
	        mPosY = in.readInt();
			offsetX = in.readInt();
			offsetY = in.readInt();
	        mCurrentWidth = in.readInt();
	        mCurrentHeight = in.readInt();
	        mHScale = in.readFloat();
	        mVScale = in.readFloat();
	        mActualScale = in.readFloat();
			mRotation = in.readInt();
			mFocused = in.readInt();
			mVisibleFrame.readFromParcel(in);
			mSystemDecorRect.readFromParcel(in);
			mDecorFrame.readFromParcel( in);
	    }

		public void dumpString(){
			LOGD("MultiWindowInfo:mPosXY=("+mPosX+","+mPosY+")"+
				"offseXY=("+offsetX+","+offsetY+")"+
				"mCurrentSize=("+mCurrentWidth+","+mCurrentHeight+")"+
				"mVHScale=("+mHScale+","+mVScale+")"+" mActualScale="+mActualScale+
				"mRotation="+mRotation+"mVisibleFrame="+mVisibleFrame+"mSystemDecorRect="+mSystemDecorRect+"mFocused="+mFocused);
		}
		@Override
		public String toString(){
			return ("MultiWindowInfo:mPosXY=("+mPosX+","+mPosY+")"+
				"offseXY=("+offsetX+","+offsetY+")"+
				"mCurrentSize=("+mCurrentWidth+","+mCurrentHeight+")"+
				"mVHScale=("+mHScale+","+mVScale+")"+" mActualScale="+mActualScale+
				"mRotation="+mRotation+"mVisibleFrame="+mVisibleFrame+"mSystemDecorRect="+mSystemDecorRect+"mFocused="+mFocused+
				",mDecorFrame:"+mDecorFrame);
		}
	}

