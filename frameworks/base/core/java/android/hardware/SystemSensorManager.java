    /*
 * Copyright (C) 2012 The Android Open Source Project
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

package android.hardware;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.MessageQueue;
import android.util.Log;
import android.util.SparseArray;
import android.util.SparseBooleanArray;
import android.util.SparseIntArray;
import dalvik.system.CloseGuard;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import android.os.Message;
import android.os.Process;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.view.IWindowManager;
/**
 * Sensor manager implementation that communicates with the built-in
 * system sensors.
 *
 * @hide
 */
public class SystemSensorManager extends SensorManager {
    private static native void nativeClassInit();
    private static native int nativeGetNextSensor(Sensor sensor, int next);

    private static boolean sSensorModuleInitialized = false;
    private static final Object sSensorModuleLock = new Object();
    private static final ArrayList<Sensor> sFullSensorsList = new ArrayList<Sensor>();
    private static final SparseArray<Sensor> sHandleToSensor = new SparseArray<Sensor>();

    // Listener list
    private final HashMap<SensorEventListener, SensorEventQueue> mSensorListeners =
            new HashMap<SensorEventListener, SensorEventQueue>();
    private final HashMap<TriggerEventListener, TriggerEventQueue> mTriggerListeners =
            new HashMap<TriggerEventListener, TriggerEventQueue>();

    // Looper associated with the context in which this instance was created.
    private final Looper mMainLooper;
    private final int mTargetSdkLevel;

   //$_rbox_$_modify_$_chenxiao_begin,add for remotecontrol
   private static final int HAS_GSENSOR = 0x01;
   private static final int HAS_GYROSCOPE = 0x10;
   private static final int HAS_MAGNETIC = 0x100;
   private static final int HAS_ORIENTATION = 0x1000;

   private static RemoteSensorThread sRemoteSensorThread;
   private static ISensorManager sSensorManager;
   private static int sRemoteSensorQueue;
   private static boolean mRemoteGSensorabled;
   private static boolean mRemoteGyroscopeabled;
   private static boolean mRemoteMagneticabled;
   private static boolean mRemoteOrientationabled;
   private static int mRemoteSensorType;

   static final ArrayList<ListenerDelegate> sListeners =
   new ArrayList<ListenerDelegate>();

   // Common pool of sensor events.
   static SensorEventPool sPool;

   static private class RemoteSensorThread {
       Thread mRemoteSensorThread;
       boolean mRemoteSensorReady;

       RemoteSensorThread() {
       }

       @Override
       protected void finalize() {
       }

       // must be called with sListeners lock
       boolean startLocked() {
           try {
               if (mRemoteSensorThread == null) {
                   mRemoteSensorReady = false;
                   RemoteSensorThreadRunnable runnable = new RemoteSensorThreadRunnable();
                   Thread thread = new Thread(runnable, RemoteSensorThread.class.getName());
                   thread.start();
                   synchronized (runnable) {
                       while (mRemoteSensorReady == false) {
                           runnable.wait();
                       }
                   }
                   mRemoteSensorThread = thread;
               }
           } catch (InterruptedException e) {
           }

           return (mRemoteSensorThread == null)? false : true;
       }

       private boolean openRemoteSensor() {
	   if(sRemoteSensorQueue==0){
		   try{
			   sRemoteSensorQueue = sSensorManager.createSensorQueue();
		   }catch(RemoteException re){
			   return false;
		   }
	   }
	   return true;
       }

       private void closeRemoteSensor(){
	   if(sRemoteSensorQueue!=0){
		   try{
			   sSensorManager.destroySensorQueue(sRemoteSensorQueue);
			   sRemoteSensorQueue = 0;
		   }catch(RemoteException re){

		   }
	   }
       }

       private int getRemoteSensorType(){
	   try {
		if(sSensorManager!=null) {
		    return sSensorManager.getRemoteSensorType();
		}
	}catch(RemoteException re){

	}
	return 0;
    }

    private void sleep(){
         try{
	    Thread.sleep(1000);
	 }catch(Exception re){
	 }
    }

    private class RemoteSensorThreadRunnable implements Runnable {
           RemoteSensorThreadRunnable() {
           }

           public void run() {
               //Log.d(TAG, "entering main sensor thread");
               float[] values = new float[3];
               final int[] status = new int[1];
               final long timestamp[] = new long[1];
               Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_DISPLAY);

               synchronized (this) {
                   // we've open the driver, we're ready to open the sensors
                   mRemoteSensorReady = true;
                   this.notify();
               }

               while (true) {
                   // wait for an event
//                     Log.d("sensorManager","remote sensor");
//                     Log.d("sensorManager","sensor listeners:"+sListeners.size());
                   int sensor = 0;
		   mRemoteSensorType = getRemoteSensorType();
                   if (mRemoteSensorType != 0) {
		    //Log.d("SensorManager","RemoteSensorEnabled");
                   openRemoteSensor();
		   SensorParcel sensorParcel = null;
		   try {
			   sensorParcel = sSensorManager.obtainSensorEvent(sRemoteSensorQueue);
		   } catch (RemoteException re){
		   }

		   if (sensorParcel!=null) {
			   values = sensorParcel.values;
			   status[0] = sensorParcel.accuracy;
			   timestamp[0] = sensorParcel.timestamp;
			   sensor = sensorParcel.sensorType;
			   //Log.d("sensorManager","sensor values:"+values[0]+","+values[1]+","+values[2]);
		   } else {
			   sleep();
			   if (!sListeners.isEmpty()) {
				   continue;
			   }
                       }
                  } else {
			sleep();
			if(!sListeners.isEmpty()){
                        continue;
                       }
                   }

                   int accuracy = status[0];
                   synchronized (sListeners) {
                       if (sensor == -1 || sListeners.isEmpty()) {
                           // we lost the connection to the event stream. this happens
                           // when the last listener is removed or if there is an error
                           if (sensor == -1 && !sListeners.isEmpty()) {
                               // log a warning in case of abnormal termination
                               Log.e(TAG, "_sensors_data_poll() failed, we bail out: sensors=" + sensor);
                           }
                           // we have no more listeners or polling failed, terminate the thread
                           // sensors_destroy_queue(sQueue);
                           // sQueue = 0;
                           closeRemoteSensor();
                           mRemoteSensorThread = null;
                           break;
                       }
                       final Sensor sensorObject = sHandleToSensor.get(sensor);
                       if (sensorObject != null) {
                           // report the sensor event to all listeners that
                           // care about it.
                           final int size = sListeners.size();
                           for (int i=0 ; i<size ; i++) {
                               ListenerDelegate listener = sListeners.get(i);
                               if (listener.hasSensor(sensorObject)) {
                                   // this is asynchronous (okay to call
                                   // with sListeners lock held).
                                   listener.onSensorChangedLocked(sensorObject,
                                           values, timestamp, accuracy);
                               }
                           }
                       }
                   }
               }
               //Log.d(TAG, "exiting main sensor thread");
           }
       }
   }


private class ListenerDelegate {
	private final SensorEventListener mSensorEventListener;
	private final ArrayList<Sensor> mSensorList = new ArrayList<Sensor>();
	private final Handler mHandler;
	public SparseBooleanArray mSensors = new SparseBooleanArray();
	public SparseBooleanArray mFirstEvent = new SparseBooleanArray();
	public SparseIntArray mSensorAccuracies = new SparseIntArray();

	ListenerDelegate(SensorEventListener listener, Sensor sensor, Handler handler) {
		mSensorEventListener = listener;
		Looper looper = (handler != null) ? handler.getLooper() : mMainLooper;
		// currently we create one Handler instance per listener, but we could
		// have one per looper (we'd need to pass the ListenerDelegate
		// instance to handleMessage and keep track of them separately).
		mHandler = new Handler(looper) {
			@Override
			public void handleMessage(Message msg) {
				final SensorEvent t = (SensorEvent)msg.obj;
				final int handle = t.sensor.getHandle();

				switch (t.sensor.getType()) {
					// Only report accuracy for sensors that support it.
					case Sensor.TYPE_MAGNETIC_FIELD:
					case Sensor.TYPE_ORIENTATION:
						// call onAccuracyChanged() only if the value changes
						final int accuracy = mSensorAccuracies.get(handle);
						if ((t.accuracy >= 0) && (accuracy != t.accuracy)) {
							mSensorAccuracies.put(handle, t.accuracy);
							mSensorEventListener.onAccuracyChanged(t.sensor, t.accuracy);
						}
						break;
					default:
						// For other sensors, just report the accuracy once
						if (mFirstEvent.get(handle) == false) {
							mFirstEvent.put(handle, true);
							mSensorEventListener.onAccuracyChanged(
									t.sensor, SENSOR_STATUS_ACCURACY_HIGH);
						}
						break;
				}

				mSensorEventListener.onSensorChanged(t);
				sPool.returnToPool(t);
			}
		};
		addSensor(sensor);
	}

	Object getListener() {
		return mSensorEventListener;
	}

	void addSensor(Sensor sensor) {
		mSensors.put(sensor.getHandle(), true);
		mSensorList.add(sensor);
	}
	int removeSensor(Sensor sensor) {
		mSensors.delete(sensor.getHandle());
		mSensorList.remove(sensor);
		return mSensors.size();
	}
	boolean hasSensor(Sensor sensor) {
		return mSensors.get(sensor.getHandle());
	}
	List<Sensor> getSensors() {
		return mSensorList;
	}

	void onSensorChangedLocked(Sensor sensor, float[] values, long[] timestamp, int accuracy) {
		SensorEvent t = sPool.getFromPool();
		final float[] v = t.values;
		v[0] = values[0];
		v[1] = values[1];
		v[2] = values[2];
		t.timestamp = timestamp[0];
		t.accuracy = accuracy;
		t.sensor = sensor;
		Message msg = Message.obtain();
		msg.what = 0;
		msg.obj = t;
		msg.setAsynchronous(true);
		mHandler.sendMessage(msg);
	}
}
   /**
    * Sensor event pool implementation.
    * @hide
    */
   protected static final class SensorEventPool {
       private final int mPoolSize;
       private final SensorEvent mPool[];
       private int mNumItemsInPool;

       private SensorEvent createSensorEvent() {
           // maximal size for all legacy events is 3
           return new SensorEvent(3);
       }

       SensorEventPool(int poolSize) {
           mPoolSize = poolSize;
           mNumItemsInPool = poolSize;
           mPool = new SensorEvent[poolSize];
       }

       SensorEvent getFromPool() {
           SensorEvent t = null;
           synchronized (this) {
               if (mNumItemsInPool > 0) {
                   // remove the "top" item from the pool
                   final int index = mPoolSize - mNumItemsInPool;
                   t = mPool[index];
                   mPool[index] = null;
                   mNumItemsInPool--;
               }
           }
           if (t == null) {
               // the pool was empty or this item was removed from the pool for
               // the first time. In any case, we need to create a new item.
               t = createSensorEvent();
           }
           return t;
       }

       void returnToPool(SensorEvent t) {
           synchronized (this) {
               // is there space left in the pool?
               if (mNumItemsInPool < mPoolSize) {
                   // if so, return the item to the pool
                   mNumItemsInPool++;
                   final int index = mPoolSize - mNumItemsInPool;
                   mPool[index] = t;
               }
           }
       }
   }

private boolean enableSensorLocked(Sensor sensor, int delay) {
       boolean result = false;
       for (ListenerDelegate i : sListeners) {
           if (i.hasSensor(sensor)) {
               String name = sensor.getName();
               int handle = sensor.getHandle();

               //result = sensors_enable_sensor(sQueue, name, handle, delay);
               if ((sensor.getType() == Sensor.TYPE_ACCELEROMETER
				|| sensor.getType() == Sensor.TYPE_GYROSCOPE
				|| sensor.getType() == Sensor.TYPE_ROTATION_VECTOR) && sSensorManager != null){
                      result = true;
               }
               break;
           }
       }
       return result;
   }

   private boolean disableSensorLocked(Sensor sensor) {
       for (ListenerDelegate i : sListeners) {
           if (i.hasSensor(sensor)) {
               // not an error, it's just that this sensor is still in use
               return true;
           }
       }
       String name = sensor.getName();
       int handle = sensor.getHandle();
       return true;//sensors_enable_sensor(sQueue, name, handle, SENSOR_DISABLE);
   }

static private void updateRemoteSensorStatus(){
	mRemoteGSensorabled = (( mRemoteSensorType & HAS_GSENSOR)  != 0);
	mRemoteGyroscopeabled = (( mRemoteSensorType & HAS_GYROSCOPE)  != 0);
}
   //$_rbox_$_modify_$_end
    /** {@hide} */
    public SystemSensorManager(Context context, Looper mainLooper) {
        mMainLooper = mainLooper;
        mTargetSdkLevel = context.getApplicationInfo().targetSdkVersion;
        synchronized(sSensorModuleLock) {
            if (!sSensorModuleInitialized) {
                sSensorModuleInitialized = true;

                nativeClassInit();

                // initialize the sensor list
                final ArrayList<Sensor> fullList = sFullSensorsList;
                int i = 0;
                do {
                    Sensor sensor = new Sensor();
                    i = nativeGetNextSensor(sensor, i);
                    if (i>=0) {
                        //Log.d(TAG, "found sensor: " + sensor.getName() +
                        //        ", handle=" + sensor.getHandle());
                        fullList.add(sensor);
                        sHandleToSensor.append(sensor.getHandle(), sensor);
                    }
                } while (i>0);
	//$_rbox_$_modify_$_chenxiao_begin,add for remotecontrol
			sPool = new SensorEventPool( sFullSensorsList.size()*2 );
			sRemoteSensorThread = new RemoteSensorThread();

			IWindowManager sWindowManager = IWindowManager.Stub.asInterface(
                       ServiceManager.getService("window"));
			if (sWindowManager != null) {
				try {
                      sSensorManager = sWindowManager.getRemoteSensorManager();
				   Log.d(TAG,"aidl getSensorManager:"+sSensorManager);
                   } catch (RemoteException e) {
					// TODO: handle exception
				}
			}
               //$_rbox_$_modify_$_end
           }
       }
   }


    /** @hide */
    @Override
    protected List<Sensor> getFullSensorList() {
        return sFullSensorsList;
    }


    /** @hide */
    @Override
    protected boolean registerListenerImpl(SensorEventListener listener, Sensor sensor,
            int delayUs, Handler handler, int maxBatchReportLatencyUs, int reservedFlags) {
        if (listener == null || sensor == null) {
            Log.e(TAG, "sensor or listener is null");
            return false;
        }
        // Trigger Sensors should use the requestTriggerSensor call.
        if (sensor.getReportingMode() == Sensor.REPORTING_MODE_ONE_SHOT) {
            Log.e(TAG, "Trigger Sensors should use the requestTriggerSensor.");
            return false;
        }
        if (maxBatchReportLatencyUs < 0 || delayUs < 0) {
            Log.e(TAG, "maxBatchReportLatencyUs and delayUs should be non-negative");
            return false;
        }

	//$_rbox_$_modify_$_chenxiao_begin,add for remotecontrol
	synchronized (sListeners) {
           // look for this listener in our list
           ListenerDelegate l = null;
           for (ListenerDelegate i : sListeners) {
               if (i.getListener() == listener) {
                   l = i;
                   break;
               }
           }

           // if we don't find it, add it to the list
           if (l == null) {
               l = new ListenerDelegate(listener, sensor, handler);
               sListeners.add(l);
               // if the list is not empty, start our main thread
               if (!sListeners.isEmpty()) {
                   if (sRemoteSensorThread.startLocked()) {
                       if (!enableSensorLocked(sensor, delayUs)) {
                           // oops. there was an error
                           sListeners.remove(l);
                       }
                   } else {
                       // there was an error, remove the listener
                       sListeners.remove(l);
                   }
               } else {
                   // weird, we couldn't add the listener
               }
           } else if (!l.hasSensor(sensor)) {
               l.addSensor(sensor);
               if (!enableSensorLocked(sensor, delayUs)) {
                   // oops. there was an error
                   l.removeSensor(sensor);
               }
           }
       }
	//$_rbox_$_modify_$_end
        // Invariants to preserve:
        // - one Looper per SensorEventListener
        // - one Looper per SensorEventQueue
        // We map SensorEventListener to a SensorEventQueue, which holds the looper
        synchronized (mSensorListeners) {
            SensorEventQueue queue = mSensorListeners.get(listener);
            if (queue == null) {
                Looper looper = (handler != null) ? handler.getLooper() : mMainLooper;
                queue = new SensorEventQueue(listener, looper, this);
                if (!queue.addSensor(sensor, delayUs, maxBatchReportLatencyUs, reservedFlags)) {
                    queue.dispose();
                    return false;
                }
                mSensorListeners.put(listener, queue);
                return true;
            } else {
                return queue.addSensor(sensor, delayUs, maxBatchReportLatencyUs, reservedFlags);
            }
        }
    }

    /** @hide */
    @Override
    protected void unregisterListenerImpl(SensorEventListener listener, Sensor sensor) {
        // Trigger Sensors should use the cancelTriggerSensor call.
        if (sensor != null && sensor.getReportingMode() == Sensor.REPORTING_MODE_ONE_SHOT) {
            return;
        }
		//$_rbox_$_modify_$_chenxiao_begin,add for remotecontrol
	synchronized (sListeners) {
           final int size = sListeners.size();
           for (int i=0 ; i<size ; i++) {
               ListenerDelegate l = sListeners.get(i);
               if (l.getListener() == listener) {
                   if (sensor == null) {
                       sListeners.remove(i);
                       // disable all sensors for this listener
                       for (Sensor s : l.getSensors()) {
                           disableSensorLocked(s);
                       }
                   } else if (l.removeSensor(sensor) == 0) {
                       // if we have no more sensors enabled on this listener,
                       // take it off the list.
                       sListeners.remove(i);
                       disableSensorLocked(sensor);
                   }
                   break;
               }
           }
       }
	//$_rbox_$_modify_$_end

        synchronized (mSensorListeners) {
            SensorEventQueue queue = mSensorListeners.get(listener);
            if (queue != null) {
                boolean result;
                if (sensor == null) {
                    result = queue.removeAllSensors();
                } else {
                    result = queue.removeSensor(sensor, true);
                }
                if (result && !queue.hasSensors()) {
                    mSensorListeners.remove(listener);
                    queue.dispose();
                }
            }
        }
    }

    /** @hide */
    @Override
    protected boolean requestTriggerSensorImpl(TriggerEventListener listener, Sensor sensor) {
        if (sensor == null) throw new IllegalArgumentException("sensor cannot be null");

        if (sensor.getReportingMode() != Sensor.REPORTING_MODE_ONE_SHOT) return false;

        synchronized (mTriggerListeners) {
            TriggerEventQueue queue = mTriggerListeners.get(listener);
            if (queue == null) {
                queue = new TriggerEventQueue(listener, mMainLooper, this);
                if (!queue.addSensor(sensor, 0, 0, 0)) {
                    queue.dispose();
                    return false;
                }
                mTriggerListeners.put(listener, queue);
                return true;
            } else {
                return queue.addSensor(sensor, 0, 0, 0);
            }
        }
    }

    /** @hide */
    @Override
    protected boolean cancelTriggerSensorImpl(TriggerEventListener listener, Sensor sensor,
            boolean disable) {
        if (sensor != null && sensor.getReportingMode() != Sensor.REPORTING_MODE_ONE_SHOT) {
            return false;
        }
        synchronized (mTriggerListeners) {
            TriggerEventQueue queue = mTriggerListeners.get(listener);
            if (queue != null) {
                boolean result;
                if (sensor == null) {
                    result = queue.removeAllSensors();
                } else {
                    result = queue.removeSensor(sensor, disable);
                }
                if (result && !queue.hasSensors()) {
                    mTriggerListeners.remove(listener);
                    queue.dispose();
                }
                return result;
            }
            return false;
        }
    }

    protected boolean flushImpl(SensorEventListener listener) {
        if (listener == null) throw new IllegalArgumentException("listener cannot be null");

        synchronized (mSensorListeners) {
            SensorEventQueue queue = mSensorListeners.get(listener);
            if (queue == null) {
                return false;
            } else {
                return (queue.flush() == 0);
            }
        }
    }

    /*
     * BaseEventQueue is the communication channel with the sensor service,
     * SensorEventQueue, TriggerEventQueue are subclases and there is one-to-one mapping between
     * the queues and the listeners.
     */
    private static abstract class BaseEventQueue {
        private native long nativeInitBaseEventQueue(BaseEventQueue eventQ, MessageQueue msgQ,
                float[] scratch);
        private static native int nativeEnableSensor(long eventQ, int handle, int rateUs,
                int maxBatchReportLatencyUs, int reservedFlags);
        private static native int nativeDisableSensor(long eventQ, int handle);
        private static native void nativeDestroySensorEventQueue(long eventQ);
        private static native int nativeFlushSensor(long eventQ);
        private long nSensorEventQueue;
        private final SparseBooleanArray mActiveSensors = new SparseBooleanArray();
        protected final SparseIntArray mSensorAccuracies = new SparseIntArray();
        protected final SparseBooleanArray mFirstEvent = new SparseBooleanArray();
        private final CloseGuard mCloseGuard = CloseGuard.get();
        private final float[] mScratch = new float[16];
        protected final SystemSensorManager mManager;

        BaseEventQueue(Looper looper, SystemSensorManager manager) {
            nSensorEventQueue = nativeInitBaseEventQueue(this, looper.getQueue(), mScratch);
            mCloseGuard.open("dispose");
            mManager = manager;
        }

        public void dispose() {
            dispose(false);
        }

        public boolean addSensor(
                Sensor sensor, int delayUs, int maxBatchReportLatencyUs, int reservedFlags) {
            // Check if already present.
            int handle = sensor.getHandle();
            if (mActiveSensors.get(handle)) return false;

            // Get ready to receive events before calling enable.
            mActiveSensors.put(handle, true);
            addSensorEvent(sensor);
            if (enableSensor(sensor, delayUs, maxBatchReportLatencyUs, reservedFlags) != 0) {
                // Try continuous mode if batching fails.
                if (maxBatchReportLatencyUs == 0 ||
                    maxBatchReportLatencyUs > 0 && enableSensor(sensor, delayUs, 0, 0) != 0) {
                  removeSensor(sensor, false);
                  return false;
                }
            }
            return true;
        }

        public boolean removeAllSensors() {
            for (int i=0 ; i<mActiveSensors.size(); i++) {
                if (mActiveSensors.valueAt(i) == true) {
                    int handle = mActiveSensors.keyAt(i);
                    Sensor sensor = sHandleToSensor.get(handle);
                    if (sensor != null) {
                        disableSensor(sensor);
                        mActiveSensors.put(handle, false);
                        removeSensorEvent(sensor);
                    } else {
                        // it should never happen -- just ignore.
                    }
                }
            }
            return true;
        }

        public boolean removeSensor(Sensor sensor, boolean disable) {
            final int handle = sensor.getHandle();
            if (mActiveSensors.get(handle)) {
                if (disable) disableSensor(sensor);
                mActiveSensors.put(sensor.getHandle(), false);
                removeSensorEvent(sensor);
                return true;
            }
            return false;
        }

        public int flush() {
            if (nSensorEventQueue == 0) throw new NullPointerException();
            return nativeFlushSensor(nSensorEventQueue);
        }

        public boolean hasSensors() {
            // no more sensors are set
            return mActiveSensors.indexOfValue(true) >= 0;
        }

        @Override
        protected void finalize() throws Throwable {
            try {
                dispose(true);
            } finally {
                super.finalize();
            }
        }

        private void dispose(boolean finalized) {
            if (mCloseGuard != null) {
                if (finalized) {
                    mCloseGuard.warnIfOpen();
                }
                mCloseGuard.close();
            }
            if (nSensorEventQueue != 0) {
                nativeDestroySensorEventQueue(nSensorEventQueue);
                nSensorEventQueue = 0;
            }
        }

        private int enableSensor(
                Sensor sensor, int rateUs, int maxBatchReportLatencyUs, int reservedFlags) {
            if (nSensorEventQueue == 0) throw new NullPointerException();
            if (sensor == null) throw new NullPointerException();
            return nativeEnableSensor(nSensorEventQueue, sensor.getHandle(), rateUs,
                    maxBatchReportLatencyUs, reservedFlags);
        }

        private int disableSensor(Sensor sensor) {
            if (nSensorEventQueue == 0) throw new NullPointerException();
            if (sensor == null) throw new NullPointerException();
            return nativeDisableSensor(nSensorEventQueue, sensor.getHandle());
        }
        protected abstract void dispatchSensorEvent(int handle, float[] values, int accuracy,
                long timestamp);
        protected abstract void dispatchFlushCompleteEvent(int handle);

        protected abstract void addSensorEvent(Sensor sensor);
        protected abstract void removeSensorEvent(Sensor sensor);
    }

    static final class SensorEventQueue extends BaseEventQueue {
        private final SensorEventListener mListener;
        private final SparseArray<SensorEvent> mSensorsEvents = new SparseArray<SensorEvent>();

        public SensorEventQueue(SensorEventListener listener, Looper looper,
                SystemSensorManager manager) {
            super(looper, manager);
            mListener = listener;
        }

        @Override
        public void addSensorEvent(Sensor sensor) {
            SensorEvent t = new SensorEvent(Sensor.getMaxLengthValuesArray(sensor,
                    mManager.mTargetSdkLevel));
            synchronized (mSensorsEvents) {
                mSensorsEvents.put(sensor.getHandle(), t);
            }
        }

        @Override
        public void removeSensorEvent(Sensor sensor) {
            synchronized (mSensorsEvents) {
                mSensorsEvents.delete(sensor.getHandle());
            }
        }

        // Called from native code.
        @SuppressWarnings("unused")
        @Override
        protected void dispatchSensorEvent(int handle, float[] values, int inAccuracy,
                long timestamp) {
            final Sensor sensor = sHandleToSensor.get(handle);
            SensorEvent t = null;
            synchronized (mSensorsEvents) {
                t = mSensorsEvents.get(handle);
            }

            if (t == null) {
                // This may happen if the client has unregistered and there are pending events in
                // the queue waiting to be delivered. Ignore.
                return;
            }
    //$_rbox_$_modify_$_chenxiao_begin,add for remotecontrol
           updateRemoteSensorStatus();
		if(sensor.getType() == Sensor.TYPE_ACCELEROMETER && mRemoteGSensorabled){
			return;
		} else if (sensor.getType() == Sensor.TYPE_GYROSCOPE && mRemoteGyroscopeabled){
			return;
		} else if(sensor.getType() == Sensor.TYPE_ROTATION_VECTOR && mRemoteGyroscopeabled){
			return;
		}
		//$_rbox_$_modify_$_end
            // Copy from the values array.
            System.arraycopy(values, 0, t.values, 0, t.values.length);
            t.timestamp = timestamp;
            t.accuracy = inAccuracy;
            t.sensor = sensor;

            // call onAccuracyChanged() only if the value changes
            final int accuracy = mSensorAccuracies.get(handle);
            if ((t.accuracy >= 0) && (accuracy != t.accuracy)) {
                mSensorAccuracies.put(handle, t.accuracy);
                mListener.onAccuracyChanged(t.sensor, t.accuracy);
            }
            mListener.onSensorChanged(t);
        }

        @SuppressWarnings("unused")
        protected void dispatchFlushCompleteEvent(int handle) {
            if (mListener instanceof SensorEventListener2) {
                final Sensor sensor = sHandleToSensor.get(handle);
                ((SensorEventListener2)mListener).onFlushCompleted(sensor);
            }
            return;
        }
    }

    static final class TriggerEventQueue extends BaseEventQueue {
        private final TriggerEventListener mListener;
        private final SparseArray<TriggerEvent> mTriggerEvents = new SparseArray<TriggerEvent>();

        public TriggerEventQueue(TriggerEventListener listener, Looper looper,
                SystemSensorManager manager) {
            super(looper, manager);
            mListener = listener;
        }

        @Override
        public void addSensorEvent(Sensor sensor) {
            TriggerEvent t = new TriggerEvent(Sensor.getMaxLengthValuesArray(sensor,
                    mManager.mTargetSdkLevel));
            synchronized (mTriggerEvents) {
                mTriggerEvents.put(sensor.getHandle(), t);
            }
        }

        @Override
        public void removeSensorEvent(Sensor sensor) {
            synchronized (mTriggerEvents) {
                mTriggerEvents.delete(sensor.getHandle());
            }
        }

        // Called from native code.
        @SuppressWarnings("unused")
        @Override
        protected void dispatchSensorEvent(int handle, float[] values, int accuracy,
                long timestamp) {
            final Sensor sensor = sHandleToSensor.get(handle);
            TriggerEvent t = null;
            synchronized (mTriggerEvents) {
                t = mTriggerEvents.get(handle);
            }
            if (t == null) {
                Log.e(TAG, "Error: Trigger Event is null for Sensor: " + sensor);
                return;
            }

            // Copy from the values array.
            System.arraycopy(values, 0, t.values, 0, t.values.length);
            t.timestamp = timestamp;
            t.sensor = sensor;

            // A trigger sensor is auto disabled. So just clean up and don't call native
            // disable.
            mManager.cancelTriggerSensorImpl(mListener, sensor, false);

            mListener.onTrigger(t);
        }

        @SuppressWarnings("unused")
        protected void dispatchFlushCompleteEvent(int handle) {
        }
    }
}
