/*
* this file is defined by hh@rock-chips.com for getting more video information
*/

package android.media;

import android.os.Parcel;
import android.os.Parcelable;

import android.util.Log;

public class VideoTrackInfor implements Parcelable
{
	private final static String  TAG = "VideoTrackInfor";
	

	public String mName = "unKnow";
	public int mWidth = 0;
	public int mHeight = 0;
	public int mFrameRate = -1;

	private VideoTrackInfor(Parcel in)	
	{		
		mName = in.readString();
		mWidth = in.readInt();		
		mHeight = in.readInt();		
		mFrameRate = in.readInt();
	}

	public static final Parcelable.Creator<VideoTrackInfor> CREATOR =            
		new Parcelable.Creator<VideoTrackInfor>()     	
	{       	
		public VideoTrackInfor createFromParcel(Parcel in)         	
		{            	
			return new VideoTrackInfor(in);        	
		}  
		
		public VideoTrackInfor[] newArray(int size) 		
		{            	
			return new VideoTrackInfor[size];        	
		}    	
	};		

	public void writeToParcel(Parcel out, int flags) 	
	{      
		out.writeString(mName);
		out.writeInt(mWidth);		
		out.writeInt(mHeight);		
		out.writeInt(mFrameRate);   	
	}	  

	public int describeContents() 	
	{        	
		return 0;    	
	}

	public void tostring()
	{
		Log.d(TAG,"mName = "+mName);
		Log.d(TAG,"mWidth = "+mWidth+",mHeight = "+mHeight);
		Log.d(TAG,"Frame Rate = "+mFrameRate);
	}
}
