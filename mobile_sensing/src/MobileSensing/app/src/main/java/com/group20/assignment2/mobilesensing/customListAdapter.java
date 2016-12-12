package com.group20.assignment2.mobilesensing;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import java.util.List;

/**
 * Created by Gaurav on 1-Dec-15.
 */
class customListAdapter extends ArrayAdapter<WifiAP> {

    public customListAdapter(Context context, List<WifiAP> objects) {
        super(context, R.layout.list_sensors_row, objects);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        LayoutInflater layoutInflater = LayoutInflater.from(getContext());
        View customRowView = layoutInflater.inflate(R.layout.list_sensors_row, parent, false);

        WifiAP wifiItem = getItem(position);

        TextView tvSSID = (TextView) customRowView.findViewById(R.id.tvSSID);
        TextView tvStrength = (TextView) customRowView.findViewById(R.id.tvStrength);

        tvSSID.setText(wifiItem.SSID);
        tvStrength.setText("BSSID: " + wifiItem.BSSID + " (" + wifiItem.level + " dBm)");
        //return super.getView(position, customRowView, parent);
        return customRowView;
    }
}