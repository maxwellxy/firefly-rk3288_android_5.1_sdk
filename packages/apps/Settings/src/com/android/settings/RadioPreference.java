/*$_FOR_ROCKCHIP_RBOX_$*/

package com.android.settings;

import android.app.Service;
import android.content.Context;  
import android.content.SharedPreferences;  
import android.content.res.TypedArray;  
import android.os.Parcel;  
import android.os.Parcelable;  
import android.preference.Preference;  
import android.util.AttributeSet;  
import android.view.View;  
import android.view.accessibility.AccessibilityEvent;  
import android.view.accessibility.AccessibilityManager;  
import android.widget.Checkable;  

public class RadioPreference extends Preference {  
  
    private CharSequence mSummaryOn;  
    private CharSequence mSummaryOff;  
    private boolean mSendAccessibilityEventViewClickedType;  
    private AccessibilityManager mAccessibilityManager;  
    private boolean mChecked;  
  
    private boolean mDisableDependentsState;  
  
    public RadioPreference(Context context, AttributeSet attrs, int defStyle) {  
        super(context, attrs, defStyle);  
        mAccessibilityManager = (AccessibilityManager) getContext()  
                .getSystemService(Service.ACCESSIBILITY_SERVICE);  
    }  
  
    public RadioPreference(Context context, AttributeSet attrs) {  
        super(context, attrs);  
        mAccessibilityManager = (AccessibilityManager) getContext()  
                .getSystemService(Service.ACCESSIBILITY_SERVICE);  
    }  
  
    public RadioPreference(Context context) {  
        super(context,null);  
    }  
  
    @Override  
    public boolean isPersistent() {  
        return false;  
    }  
  
    @Override  
    protected void onBindView(View view) {  
        super.onBindView(view);  
        View radioView = view.findViewById(R.id.radiobutton);  
        if (radioView != null && radioView instanceof Checkable) {  
            ((Checkable) radioView).setChecked(mChecked);  
            if (mSendAccessibilityEventViewClickedType
                    && mAccessibilityManager.isEnabled()  
                    && radioView.isEnabled()) {  
                mSendAccessibilityEventViewClickedType = false;  
  
                int eventType = AccessibilityEvent.TYPE_VIEW_CLICKED;  
                radioView.sendAccessibilityEventUnchecked(AccessibilityEvent
                        .obtain(eventType));  
            }  
        }  
    }  
  
    @Override  
    protected void onClick() {  
        super.onClick();  
        boolean newValue = !isChecked();  
        mSendAccessibilityEventViewClickedType = true;  
  
        if (!callChangeListener(newValue)) {  
            return;  
        }  
        setChecked(newValue);  
  
    }  
  
    /** 
     * Sets the checked state and saves it to the {@link SharedPreferences}. 
     *  
     * @param checked 
     *            The checked state. 
     */  
    public void setChecked(boolean checked) {  
        if (mChecked != checked) {  
            mChecked = checked;  
            persistBoolean(checked);  
            notifyDependencyChange(shouldDisableDependents());  
            notifyChanged();  
        }  
    }  
  
    /** 
     * Returns the checked state. 
     *  
     * @return The checked state. 
     */  
    public boolean isChecked() {  
        return mChecked;  
    }  
  
    @Override  
    public boolean shouldDisableDependents() {  
        boolean shouldDisable = mDisableDependentsState ? mChecked : !mChecked;  
        return shouldDisable || super.shouldDisableDependents();  
    }  
  
    /** 
     * Sets the summary to be shown when checked. 
     *  
     * @param summary 
     *            The summary to be shown when checked. 
     */  
    public void setSummaryOn(CharSequence summary) {  
        mSummaryOn = summary;  
        if (isChecked()) {  
            notifyChanged();  
        }  
    }  
  
    /** 
     * @see #setSummaryOn(CharSequence) 
     * @param summaryResId
     *            The summary as a resource. 
     */  
    public void setSummaryOn(int summaryResId) {  
        setSummaryOn(getContext().getString(summaryResId));  
    }  
  
    /** 
     * Returns the summary to be shown when checked. 
     *  
     * @return The summary. 
     */  
    public CharSequence getSummaryOn() {  
        return mSummaryOn;  
    }  
  
    /** 
     * Sets the summary to be shown when unchecked. 
     *  
     * @param summary 
     *            The summary to be shown when unchecked. 
     */  
    public void setSummaryOff(CharSequence summary) {  
        mSummaryOff = summary;  
        if (!isChecked()) {  
            notifyChanged();  
        }  
    }  
  
    /** 
     * @see #setSummaryOff(CharSequence) 
     * @param summaryResId
     *            The summary as a resource. 
     */  
    public void setSummaryOff(int summaryResId) {  
        setSummaryOff(getContext().getString(summaryResId));  
    }  
  
    /** 
     * Returns the summary to be shown when unchecked. 
     *  
     * @return The summary. 
     */  
    public CharSequence getSummaryOff() {  
        return mSummaryOff;  
    }  
  
    /** 
     * Returns whether dependents are disabled when this preference is on ( 
     * {@code true}) or when this preference is off ({@code false}). 
     *  
     * @return Whether dependents are disabled when this preference is on ( 
     *         {@code true}) or when this preference is off ({@code false}). 
     */  
    public boolean getDisableDependentsState() {  
        return mDisableDependentsState;  
    }  
  
    /** 
     * Sets whether dependents are disabled when this preference is on ( 
     * {@code true}) or when this preference is off ({@code false}). 
     *  
     * @param disableDependentsState
     *            The preference state that should disable dependents. 
     */  
    public void setDisableDependentsState(boolean disableDependentsState) {  
        mDisableDependentsState = disableDependentsState;  
    }  
  
    @Override  
    protected Object onGetDefaultValue(TypedArray a, int index) {  
        return a.getBoolean(index, false);  
    }  
  
    @Override  
    protected void onSetInitialValue(boolean restoreValue, Object defaultValue) {  
        setChecked(restoreValue ? getPersistedBoolean(mChecked)  
                : (Boolean) defaultValue);  
    }  
  
    @Override  
    protected Parcelable onSaveInstanceState() {  
        final Parcelable superState = super.onSaveInstanceState();  
        if (isPersistent()) {  
            // No need to save instance state since it's persistent  
            return superState;  
        }  
  
        final SavedState myState = new SavedState(superState);  
        myState.checked = isChecked();  
        return myState;  
    }  
  
    @Override  
    protected void onRestoreInstanceState(Parcelable state) {  
        if (state == null || !state.getClass().equals(SavedState.class)) {  
            // Didn't save state for us in onSaveInstanceState
            super.onRestoreInstanceState(state);  
            return;  
        }  
  
        SavedState myState = (SavedState) state;  
        super.onRestoreInstanceState(myState.getSuperState());  
        setChecked(myState.checked);  
    }  
  
    private static class SavedState extends BaseSavedState {
        boolean checked;
        
        public SavedState(Parcel source) {
            super(source);
            checked = source.readInt() == 1;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            super.writeToParcel(dest, flags);
            dest.writeInt(checked ? 1 : 0);
        }

        public SavedState(Parcelable superState) {
            super(superState);
        }

        public static final Parcelable.Creator<SavedState> CREATOR =
                new Parcelable.Creator<SavedState>() {
            public SavedState createFromParcel(Parcel in) {
                return new SavedState(in);
            }

            public SavedState[] newArray(int size) {
                return new SavedState[size];
            }
        };
    }  
}

