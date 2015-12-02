package com.group20.assignment2.mobilesensing;

import android.content.Context;
import android.content.Intent;
import android.net.wifi.WifiManager;
import android.os.Environment;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

public class MainActivity extends ActionBarActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        wifiManager.startScan();

        MyApplication.getExtStoragePath();

        //Button Click handler for "List Sensors"
        Button buttonListSensors = (Button) findViewById(R.id.buttonListSensors);
        buttonListSensors.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent listSensorsActivity = new Intent(MainActivity.this, ListSensorsActivity.class);
                startActivity(listSensorsActivity);
            }
        });

        //Button Click Handler for "Scan Wifi Points"
        Button buttonScanWifi = (Button) findViewById(R.id.buttonScanWifi);
        buttonScanWifi.setOnClickListener( new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent scanWifiActivity = new Intent(MainActivity.this, ScanWifiActivity.class);
                startActivity(scanWifiActivity);
            }
        });

        //Button Click handler for "Localization Example"
        Button buttonLocalizationExample = (Button) findViewById(R.id.buttonLocalizationExample);
        buttonLocalizationExample.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent localizationExample = new Intent(MainActivity.this, LocalizationActivity.class);
                startActivity(localizationExample);
            }
        });

    }

    @Override
    protected void onResume() {
        super.onResume();

    }

}