//$_FOR_ROCKCHIP_RBOX_$
/*$_rbox_$_modify_$_chenxiao*/
//$_rbox_$_modify_$_begin
/*******************************************************************
* Company:     Fuzhou Rockchip Electronics Co., Ltd
* Filename:    SensorParcel.java  
* Description:   
* @author:     fxw@rock-chips.com
* Create at:   2011-12-17
* 
* Modification History:  
* Date         Author      Version     Description  
* ------------------------------------------------------------------  
* 2011-12-17      xwf         1.0         create
*******************************************************************/   


package android.hardware;


import android.os.Parcel;
import android.os.Parcelable;

public class SensorParcel implements Parcelable {
	
	public float[] values;
	public int accuracy;
	public long timestamp;
	public int sensorType;

	/** 
	 * <p>Title: describeContents</p> 
	 * <p>Description: </p> 
	 * @return 
	 * @see android.os.Parcelable#describeContents() 
	 */
	@Override
	public int describeContents() {
		return 0;
	}

	@Override
	public void writeToParcel(Parcel dest, int flags) {
		dest.writeFloatArray(values);
		dest.writeInt(accuracy);
		dest.writeLong(timestamp);
		dest.writeInt(sensorType);
	}
	
	public static final Parcelable.Creator<SensorParcel> CREATOR = new Creator<SensorParcel>() {
		@Override
		public SensorParcel createFromParcel(Parcel source) {
			SensorParcel sensor = new SensorParcel();
			float[] values = new float[3];
			source.readFloatArray(values);
			sensor.values = values;
			sensor.accuracy = source.readInt();
			sensor.timestamp = source.readLong();
			sensor.sensorType = source.readInt();
			return sensor;
		}
		
		@Override
		public SensorParcel[] newArray(int size) {
			return new SensorParcel[size];
		}
	};
}
//$_rbox_$_modify_$_end
