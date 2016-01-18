package android.media;

import android.os.Parcel;
import android.os.Parcelable;

import android.util.Log;

public class SubtitleTrackInfor implements Parcelable
{
	private final static String  TAG = "SubtitleTrackInfor";

	public final static int TIMEDTEXT = 3;
	public final static int PGS = 4;
	
	public int mType  = PGS;
	public String mName = "unKnow";
	public String mLang = "unKnow";

	private SubtitleTrackInfor(Parcel in)	
	{
		mType = in.readInt();
		mName = in.readString();
		mLang = in.readString();
	}

	public static final Parcelable.Creator<SubtitleTrackInfor> CREATOR =            
		new Parcelable.Creator<SubtitleTrackInfor>()     	
	{       	
		public SubtitleTrackInfor createFromParcel(Parcel in)         	
		{            	
			return new SubtitleTrackInfor(in);        	
		}  
		
		public SubtitleTrackInfor[] newArray(int size) 		
		{            	
			return new SubtitleTrackInfor[size];        	
		}    	
	};		

	public void writeToParcel(Parcel out, int flags) 	
	{
		out.writeInt(mType);
		out.writeString(mName);
		out.writeString(mLang);	
	}	  

	public int describeContents() 	
	{        	
		return 0;    	
	}

	public void tostring()
	{
		Log.d(TAG,"Type = "+mType);
		Log.d(TAG,"mName = "+mName);
		Log.d(TAG,"mLang = "+mLang);
	}
}