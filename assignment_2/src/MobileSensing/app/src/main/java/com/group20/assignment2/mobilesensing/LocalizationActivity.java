package com.group20.assignment2.mobilesensing;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.text.InputType;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.channels.FileChannel;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.util.Scanner;

public class LocalizationActivity extends ActionBarActivity {

    private List<LocationPoint> references;


    private void buildReferenceList() {
        try {
            String[] locations = (new File(MyApplication.getExtStoragePath() + "/reference_data")).list();

            references = new ArrayList<LocationPoint>();

            for (String loc : locations) {

                String[] files = (new File(MyApplication.getExtStoragePath() + "/reference_data/" + loc)).list();
                if (files.length != 0)
                    references.add(new LocationPoint(loc));
            }

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private String scanAPGetJSON() {
        String str = null;
        JSONObject jsonObject = new JSONObject();

        try {
            JSONObject jsonMeasurement = new JSONObject();

            WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
            List<ScanResult> wifiScanResult = wifiManager.getScanResults();

            Integer i = 1;
            for (ScanResult scanResult : wifiScanResult) {
                JSONObject jsonAP = new JSONObject();
                jsonAP.put("capabilities", scanResult.capabilities);
                jsonAP.put("level", scanResult.level);
                jsonAP.put("frequency", scanResult.frequency);
                jsonAP.put("BSSID", scanResult.BSSID);
                jsonAP.put("SSID", scanResult.SSID);

                jsonMeasurement.put("ap"+i.toString(), jsonAP);
                i++;
            }

            jsonObject.put("measurement 1", jsonMeasurement);

            str = jsonObject.toString();

        } catch (JSONException e) {
            e.printStackTrace();
        }

        return str;
    }

    private String computeNearbyLocation (LocationPoint measurementPt) {
        Double dist = 999.99;
        String nearbyLoc = "Cannot be localized!";
        String strToast = "";
        for (LocationPoint ref : references) {
            Double d = ref.distanceFrom(measurementPt);

            if (Double.compare(d, dist) <= 0) {
                dist = d;
                nearbyLoc = ref.name + " (Dist: " + String.format("%.2f", d) + ")";
            }

            strToast = strToast + ref.name + " (Dist: " + String.format("%.2f", d) + ")" + "\n";
        }
        Toast.makeText(LocalizationActivity.this, strToast, Toast.LENGTH_SHORT).show();
        return nearbyLoc;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_localization);

        Spinner spinnerMeasurement = (Spinner) findViewById(R.id.spinnerMeasurement);
        //Handler for Spinner Measurement
        spinnerMeasurement.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {

            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {

                String filename = ((TextView) view).getText().toString();

                try {

                    LocationPoint measurementPoint = new LocationPoint(filename, fileToJSONString(filename));

                    ListView lvMeasurementDetails = (ListView) findViewById(R.id.lvMeasurementDetails);
                    lvMeasurementDetails.setAdapter(new customListAdapter(LocalizationActivity.this, measurementPoint.fingerprint));

                    String nearbyLocation;
                    nearbyLocation = computeNearbyLocation(measurementPoint);

                    TextView tvComputedLocation = (TextView) findViewById(R.id.tvComputedLocation);
                    tvComputedLocation.setText(nearbyLocation);

                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });

        //BUtton CREATE Handler
        Button buttonCreateMeasurement = (Button) findViewById(R.id.buttonCreatMeasurement);
        buttonCreateMeasurement.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
                wifiManager.startScan();
                AlertDialog.Builder builder = new AlertDialog.Builder(LocalizationActivity.this);
                builder.setTitle("Create Measurement");

                final EditText input = new EditText(LocalizationActivity.this);
                input.setInputType(InputType.TYPE_CLASS_TEXT);
                input.setHint("Enter Measurement Name");

                builder.setView(input);

                builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        String filename = input.getText().toString() + ".json";

                        try {
                            FileOutputStream fos = new FileOutputStream(MyApplication.getExtStoragePath() + "/measurement_data/" + filename);

                            String jsonData = scanAPGetJSON();
                            fos.write(jsonData.getBytes());
                            fos.close();

                            LocalizationActivity.this.finish();
                            startActivity(getIntent().putExtra("selection", filename));

                        } catch (FileNotFoundException e) {
                            e.printStackTrace();
                        } catch (IOException e) {
                            e.printStackTrace();
                        }

                    }
                });

                builder.create().show();
            }
        });


        //BUTTON UPDATE HANDLER
        Button buttonUpdateMeasurement = (Button) findViewById(R.id.buttonUpdateMeasurement);
        buttonUpdateMeasurement.setOnClickListener(new Button.OnClickListener(){
            @Override
            public void onClick(View v) {
                WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
                wifiManager.startScan();

                AlertDialog.Builder builder = new AlertDialog.Builder(LocalizationActivity.this);
                builder.setTitle("Are you sure you want to update the measurement?");
                builder.setMessage("Please make sure that you are at the same location. Wrong information will lead to improper results");

                builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                    }
                });

                builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        Spinner spinnerMeasurement = (Spinner) findViewById(R.id.spinnerMeasurement);
                        String filename = spinnerMeasurement.getSelectedItem().toString();

                        InputStream inputStream = null;
                        try {
                            inputStream = new FileInputStream(MyApplication.getExtStoragePath() + "/measurement_data/" + filename);
                            String jsonString = new Scanner(inputStream).useDelimiter("\\A").next();
                            inputStream.close();

                            JSONObject jsonObject = new JSONObject(jsonString);
                            Iterator<String> measurementKeys = jsonObject.keys();

                            Integer num = 0;  //number of measurements already done
                            while (measurementKeys.hasNext()) {
                                measurementKeys.next();
                                num++;
                            }

                            JSONObject jsonObjectToAppend = new JSONObject(scanAPGetJSON());

                            num++;
                            jsonObject.put("measurement " + num.toString(), jsonObjectToAppend.getJSONObject("measurement 1"));

                            //now write file
                            FileOutputStream fos = new FileOutputStream(MyApplication.getExtStoragePath() + "/measurement_data/" + filename);
                            String jsonData = jsonObject.toString();
                            fos.write(jsonData.getBytes());
                            fos.close();

                            Toast.makeText(LocalizationActivity.this, "Updated with measurement " + num.toString(), Toast.LENGTH_SHORT).show();
                            LocalizationActivity.this.finish();
                            startActivity(getIntent().putExtra("selection", filename));

                        } catch (FileNotFoundException e) {
                            e.printStackTrace();
                        } catch (IOException e) {
                            e.printStackTrace();
                        } catch (JSONException e) {
                            e.printStackTrace();
                        }
                    }
                });

                builder.create().show();

            }
        });

        //Button REFERENCE POINTS click handler
        Button buttonReference = (Button) findViewById(R.id.buttonReferencePoints);
        buttonReference.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent viewReferences = new Intent(LocalizationActivity.this, ViewReferencePointsActivity.class);
                startActivity(viewReferences);
            }
        });


        //Button SAVE REFERENCE click handler
        Button buttonSaveReference = (Button) findViewById(R.id.buttonSaveReference);
        buttonSaveReference.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog.Builder builder = new AlertDialog.Builder(LocalizationActivity.this);
                builder.setTitle("Create a reference location point");

                final EditText input = new EditText(LocalizationActivity.this);
                input.setInputType(InputType.TYPE_CLASS_TEXT);
                input.setHint("Enter location name here");

                builder.setView(input);

                builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        Spinner spinnerMeasurement = (Spinner) findViewById(R.id.spinnerMeasurement);
                        String inFilename = spinnerMeasurement.getSelectedItem().toString();

                        String dirName = input.getText().toString();

                        File copyFrom = new File(MyApplication.getExtStoragePath() + "/measurement_data/" + inFilename);

                        File dir = new File(MyApplication.getExtStoragePath() + "/reference_data/" + dirName);

                        if (!dir.exists()) {
                            if (!dir.mkdir())
                                Toast.makeText(LocalizationActivity.this, "Could not create directory!", Toast.LENGTH_SHORT).show();
                        }

                        SimpleDateFormat s = new SimpleDateFormat("yyyyMMdd_hhmmss");
                        String time_stamp = s.format(new Date());
                        String outFilename = "result_" + time_stamp;

                        File copyTo = new File(MyApplication.getExtStoragePath() + "/reference_data/" + dirName + "/" + outFilename);

                        FileChannel inputChannel = null, outputChannel = null;
                        try {
                            inputChannel = new FileInputStream(copyFrom).getChannel();
                            outputChannel = new FileOutputStream(copyTo).getChannel();
                            outputChannel.transferFrom(inputChannel, 0, inputChannel.size());
                            inputChannel.close();
                            outputChannel.close();
                        } catch (IOException e) {
                            e.printStackTrace();
                            Toast.makeText(LocalizationActivity.this, "Could not create json file!", Toast.LENGTH_SHORT).show();
                        } finally {
                            Toast.makeText(LocalizationActivity.this, "Created file - " + outFilename, Toast.LENGTH_SHORT).show();

                            //adding location to references
                            try {
                                references.add(new LocationPoint(dirName));
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                            Intent viewReferencePoints = new Intent(LocalizationActivity.this, ViewReferencePointsActivity.class);
                            viewReferencePoints.putExtra("selection", dirName);
                            startActivity(viewReferencePoints);
                        }
                    }
                });

                builder.create().show();

            }
        });

        //BUTTON Delete handler
        Button buttonDelete = (Button) findViewById(R.id.buttonDeleteMeasurement);
        buttonDelete.setOnClickListener(new Button.OnClickListener(){
            @Override
            public void onClick(View v) {
                AlertDialog.Builder builder = new AlertDialog.Builder(LocalizationActivity.this);
                builder.setTitle("Delete measurement?");
                builder.setMessage("Are you sure you want to delete this measurement?");

                builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                    }
                });

                builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        Spinner spinnerMeasurement = (Spinner) findViewById(R.id.spinnerMeasurement);
                        String filename = spinnerMeasurement.getSelectedItem().toString();

                        File file = new File(MyApplication.getExtStoragePath() + "/measurement_data/" + filename);
                        if (file.delete()) {
                            Toast.makeText(LocalizationActivity.this, "Measurement has been deleted", Toast.LENGTH_SHORT).show();
                            LocalizationActivity.this.finish();
                            startActivity(getIntent());
                        } else {
                            Toast.makeText(LocalizationActivity.this, "Could not delete measurement", Toast.LENGTH_SHORT).show();
                        }
                    }
                });

                builder.create().show();

            }
        });
    }

    protected void onResume() {
        super.onResume();

        buildReferenceList();

        String[] files = new File(MyApplication.getExtStoragePath() + "/measurement_data").list();
        Spinner spinnerMeasurement = (Spinner) findViewById(R.id.spinnerMeasurement);
        spinnerMeasurement.setAdapter(new ArrayAdapter<String>(this, android.R.layout.simple_spinner_dropdown_item, files));

        String selection;
        selection = getIntent().getStringExtra("selection");

        if (selection != null) {
            for (int i = 0; i < files.length; i++) {
                if (selection.equals(files[i])) {
                    spinnerMeasurement.setSelection(i, true);
                }
            }
        }

    }

    private String fileToJSONString (String fileName) throws IOException {
        String extStoragePath = MyApplication.getContext().getExternalFilesDir(null).getAbsolutePath();
        //AssetManager assetManager = MyApplication.getContext().getAssets();
        InputStream inputStream = new FileInputStream(extStoragePath + "/measurement_data/" + fileName);
        //InputStream inputStream = assetManager.open("measurement_data/" + fileName);
        String jsonString = new Scanner(inputStream).useDelimiter("\\A").next();

        inputStream.close();
        return jsonString;
    }
}

