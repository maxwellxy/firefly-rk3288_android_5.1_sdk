package android.media;

import android.os.Parcel;
import android.os.Parcelable;

import android.util.Log;

public class AudioTrackInfor implements Parcelable
{
	private final static String  TAG = "AudioTrackInfor";
	
	public int mSample = 0;
	public int mChannel = 0;
	public String mName = "unKnow";
	public String mLang = "unKnow";

	private AudioTrackInfor(Parcel in)	
	{		
		mSample = in.readInt();		
		mChannel = in.readInt();
		mName = in.readString();
		mLang = in.readString();
	}

	public static final Parcelable.Creator<AudioTrackInfor> CREATOR =            
		new Parcelable.Creator<AudioTrackInfor>()     	
	{       	
		public AudioTrackInfor createFromParcel(Parcel in)         	
		{            	
			return new AudioTrackInfor(in);        	
		}  
		
		public AudioTrackInfor[] newArray(int size) 		
		{            	
			return new AudioTrackInfor[size];        	
		}    	
	};		

	public void writeToParcel(Parcel out, int flags) 	
	{      
		out.writeInt(mSample);		
		out.writeInt(mChannel);	
		out.writeString(mName);
		out.writeString(mLang);
	}	  

	public int describeContents() 	
	{        	
		return 0;    	
	}

	public void tostring()
	{
		Log.d(TAG,"mSample = "+mSample+",mChannel = "+mChannel);
		Log.d(TAG,"mName = "+mName);
		Log.d(TAG,"mLang = "+mLang);
	}
}

