package android.os;  
  
import android.util.Log;  
import android.os.IInterface;  
import android.os.Binder;  
import android.os.IBinder;  
import android.os.Parcel;  
import android.os.ParcelFileDescriptor;  
import android.os.RemoteException;  
  
public interface IMemoryService extends IInterface {  
    public static abstract class Stub extends Binder implements IMemoryService {  
        private static final String DESCRIPTOR = "android.os.ashmem.IMemoryService";  
  
        public Stub() {  
            attachInterface(this, DESCRIPTOR);  
        }  
  
        public static IMemoryService asInterface(IBinder obj) {  
            if (obj == null) {  
                return null;  
            }  
  
            IInterface iin = (IInterface)obj.queryLocalInterface(DESCRIPTOR);  
            if (iin != null && iin instanceof IMemoryService) {  
                return (IMemoryService)iin;  
            }  
  
            return new IMemoryService.Stub.Proxy(obj);  
        }  
  
        public IBinder asBinder() {  
            return this;  
        }  
  
        @Override   
        public boolean onTransact(int code, Parcel data, Parcel reply, int flags) throws android.os.RemoteException {  
            switch (code) {  
            case INTERFACE_TRANSACTION: {  
                reply.writeString(DESCRIPTOR);  
                return true;  
            }  
            case TRANSACTION_getFileDescriptor: {  
                data.enforceInterface(DESCRIPTOR);  
                  
                ParcelFileDescriptor result = this.getFileDescriptor();  
                  
                reply.writeNoException();  
                  
                if (result != null) {  
                    reply.writeInt(1);  
                    result.writeToParcel(reply, 0);  
                } else {  
                    reply.writeInt(0);  
                }  
  
                return true;  
            }  
            case TRANSACTION_getLength: {  
                data.enforceInterface(DESCRIPTOR);  
                  
                int length = this.getLength();  
                  
                reply.writeNoException();  
                
                reply.writeInt(length);
                  
                return true;  
            }  
            }  
  
            return super.onTransact(code, data, reply, flags);  
        }  
  
        private static class Proxy implements IMemoryService {  
            private IBinder mRemote;  
  
            Proxy(IBinder remote) {  
                mRemote = remote;  
            }  
  
            public IBinder asBinder() {  
                return mRemote;  
            }  
  
            public String getInterfaceDescriptor() {  
                return DESCRIPTOR;  
            }  
  
            public ParcelFileDescriptor getFileDescriptor() throws RemoteException {  
                Parcel data = Parcel.obtain();  
                Parcel reply = Parcel.obtain();  
  
                ParcelFileDescriptor result;  
      
                try {  
                    data.writeInterfaceToken(DESCRIPTOR);  
  
                    mRemote.transact(Stub.TRANSACTION_getFileDescriptor, data, reply, 0);  
          
                    reply.readException();  
                    if (0 != reply.readInt()) {  
                        result = ParcelFileDescriptor.CREATOR.createFromParcel(reply);  
                    } else {  
                        result = null;  
                    }  
                } finally {  
                    reply.recycle();  
                    data.recycle();  
                }  
      
                return result;  
            }  
  
            public int getLength() throws RemoteException {  
                Parcel data = Parcel.obtain();  
                Parcel reply = Parcel.obtain();  
   
                int mLength = -1;
                
                try {  
                    data.writeInterfaceToken(DESCRIPTOR);  
  
                    mRemote.transact(Stub.TRANSACTION_getLength, data, reply, 0);  
                      
                    reply.readException();  
                    mLength = reply.readInt();
                } finally {  
                    reply.recycle();  
                    data.recycle();  
                }  
                
                return mLength;
            }  
        }  
  
        static final int TRANSACTION_getFileDescriptor = IBinder.FIRST_CALL_TRANSACTION + 0;  
        static final int TRANSACTION_getLength = IBinder.FIRST_CALL_TRANSACTION + 1;  
  
    }  
  
    public ParcelFileDescriptor getFileDescriptor() throws RemoteException;  
    public int getLength() throws RemoteException;  
}  