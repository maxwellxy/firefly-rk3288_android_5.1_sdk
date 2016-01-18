package android.view;

import android.view.KeyEvent;

/**
* {@hide}
 */
 oneway interface IAppAlignWatcher {
    void onAppAlignChanged(int align,boolean rotate);    
    void onHalfScreenWindowPositionChanged(int posX,int posY);  
    void dispatchUnhandledKey(in KeyEvent event);   
    void onTopAllWindowChanged(int taskid);
    void applyXTrac(int x);
 }
