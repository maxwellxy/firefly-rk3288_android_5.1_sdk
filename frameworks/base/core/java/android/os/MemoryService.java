package android.os;

import java.io.FileDescriptor;
import java.io.IOException;

import android.os.Parcel;
import android.os.MemoryFile;
import android.os.ParcelFileDescriptor;
import android.util.Log;

public class MemoryService extends IMemoryService.Stub {
	
	private final static String LOG_TAG = "android.os.ashmem.MemoryService";
	
	private MemoryFile file = null;
	private int mLength = -1;

	public MemoryService(byte[] src) {
		try {
			mLength = src.length;
			file = new MemoryFile("Ashmem", mLength);
			file.writeBytes(src, 0, 0, mLength);
		} catch (IOException ex) {
			Log.i(LOG_TAG, "Failed to create memory file.");
			ex.printStackTrace();
		}
	}
	
	public void setSource(byte [] src){
		try {
			if(file != null){
				file.close();
				file = null;
			}
			mLength = src.length;
			file = new MemoryFile("Ashmem", mLength);
			file.writeBytes(src, 0, 0, mLength);
		} catch (Exception ex) {
			Log.i(LOG_TAG, "Failed to reCreate memory file.");
			ex.printStackTrace();
		}
	}

	public ParcelFileDescriptor getFileDescriptor() {
		Log.i(LOG_TAG, "Get File Descriptor.");

		ParcelFileDescriptor pfd = null;

		try {
			pfd = new ParcelFileDescriptor(file.getFileDescriptor());
		} catch (IOException ex) {
			Log.i(LOG_TAG, "Failed to get file descriptor.");
			ex.printStackTrace();
		}

		return pfd;
	}
	
	public int getLength(){
		Log.i(LOG_TAG, "Get Length.");
		return mLength;
	}
}