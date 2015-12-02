package com.group20.assignment2.mobilesensing;

import android.app.Application;
import android.content.Context;

/**
 * Created by Gaurav on 30-Nov-15.
 */
public class MyApplication extends Application {

    private static Context mContext;

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = getApplicationContext();
    }

    public static Context getContext() {
        return mContext;
    }

    public static String getExtStoragePath() {
        String path = mContext.getExternalFilesDir(null).getAbsolutePath();
        return path;
    };
}