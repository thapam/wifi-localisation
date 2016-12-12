package com.group20.assignment2.mobilesensing;

import android.content.Context;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import org.w3c.dom.Text;

import java.sql.Array;
import java.util.ArrayList;
import java.util.List;

public class ScanWifiActivity extends ActionBarActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_scan_wifi);

        WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        wifiManager.startScan();

        try {
            Thread.sleep(2000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        List<ScanResult> wifiScanResult = wifiManager.getScanResults();

        ListView lvWifiPoints = (ListView) findViewById(R.id.lvWifiPoints);
        lvWifiPoints.setAdapter(new WifiPointsAdapter(this, wifiScanResult));

    }

    public class WifiPointsAdapter extends ArrayAdapter<ScanResult> {

        public WifiPointsAdapter(Context context, List<ScanResult> objects) {
            super(context, R.layout.list_sensors_row, objects);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            LayoutInflater layoutInflater = LayoutInflater.from(getContext());
            View customRowView = layoutInflater.inflate(R.layout.list_sensors_row, parent, false);

            ScanResult wifiItem = getItem(position);

            TextView tvSSID = (TextView) customRowView.findViewById(R.id.tvSSID);
            TextView tvStrength = (TextView) customRowView.findViewById(R.id.tvStrength);

            tvSSID.setText(wifiItem.SSID);
            tvStrength.setText("BSSID: " + wifiItem.BSSID + " (" + wifiItem.level + " dBm)");

            //return super.getView(position, customRowView, parent);
            return customRowView;
        }
    }
}
