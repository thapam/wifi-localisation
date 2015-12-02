package com.group20.assignment2.mobilesensing;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Scanner;

/**
 * Created by Gaurav on 30-Nov-15.
 */
public class LocationPoint {

    public String name;
    public List<WifiAP> fingerprint;

    public LocationPoint(String name, String jsonData) {
        name = name;
        buildFingerPrint(jsonData);
    }

    public LocationPoint(String name) throws IOException {
        this.name = name;
        //AssetManager assetManager = MyApplication.getContext().getAssets();

        String extStoragePath = MyApplication.getContext().getExternalFilesDir(null).getAbsolutePath();
        String[] files = new File(extStoragePath + "/reference_data/" + name).list();
        //String[] files = assetManager.list("reference_data/" + name);

        String jsonString = null;
        for (String file : files) {

            //InputStream inputStream = MyApplication.getContext().openFileInput(extStoragePath + "/reference_data/" + name + "/" + file);
            InputStream inputStream = new FileInputStream(extStoragePath + "/reference_data/" + name + "/" + file);

            //InputStream inputStream = assetManager.open("reference_data/" + name + "/" + file);
            jsonString = new Scanner(inputStream).useDelimiter("\\A").next();

            inputStream.close();
            buildFingerPrint(jsonString);
        }
    }

    private void buildFingerPrint (String jsonString) {
        this.fingerprint = new ArrayList<WifiAP>();
        try {
            JSONObject jsonObject = new JSONObject(jsonString);

            Iterator <String> measurementKeys = jsonObject.keys();

            while (measurementKeys.hasNext()) {
                JSONObject jsonMeasurement = jsonObject.getJSONObject(measurementKeys.next());

                Iterator<String> apKeys = jsonMeasurement.keys();

                while(apKeys.hasNext()) {

                    JSONObject jsonAP = jsonMeasurement.getJSONObject(apKeys.next());

                    WifiAP wifiAP = new WifiAP();
                    wifiAP.BSSID = jsonAP.getString("BSSID");
                    wifiAP.SSID = jsonAP.getString("SSID");
                    wifiAP.frequency = jsonAP.getInt("frequency");
                    wifiAP.level = jsonAP.getInt("level");
                    wifiAP.capabilities = jsonAP.getString("capabilities");

                    if (this.fingerprint.size() == 0)
                        this.fingerprint.add(wifiAP);
                    else {
                        int n = this.fingerprint.size();

                        boolean flag = false;

                        for (int i = 0; i < n; i++) {
                            if (this.fingerprint.get(i).BSSID.equals(wifiAP.BSSID) && (this.fingerprint.get(i).frequency == wifiAP.frequency)) {
                                wifiAP.level = (wifiAP.level + this.fingerprint.get(i).level) / 2;
                                this.fingerprint.set(i, wifiAP);
                                flag = true;
                                break;
                            }
                        }

                        if (flag == false) {
                            this.fingerprint.add(wifiAP);
                        }
                    }
                }
            }

        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    public double distanceFrom(LocationPoint locationPoint){

        double sum = 0;
        double distance = 99999.99;

        boolean isBSSIDMatched = false;

        double sumAvgAP;
        for (WifiAP wifiMeasurement : locationPoint.fingerprint) {

            int i = 0;
            sumAvgAP = 0;
            for (WifiAP wifiReference : this.fingerprint) {

                if (wifiMeasurement.BSSID.equals(wifiReference.BSSID)) {
                    sumAvgAP = sumAvgAP + Math.pow(Math.abs(wifiMeasurement.level) - Math.abs(wifiReference.level), 2);
                    i++;
                    isBSSIDMatched = true;
                }

            }

            if (i != 0)
                sumAvgAP = sumAvgAP / i;

            sum = sum + sumAvgAP;
        }

        if (isBSSIDMatched)
            distance = Math.sqrt(sum);

        return distance;
    }

}
