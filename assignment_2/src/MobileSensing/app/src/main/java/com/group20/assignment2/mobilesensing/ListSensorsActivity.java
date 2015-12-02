package com.group20.assignment2.mobilesensing;

import android.app.AlertDialog;
import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.os.Build;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import org.w3c.dom.Text;

import java.io.IOException;
import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.List;

public class ListSensorsActivity extends ActionBarActivity {

    SensorManager sensorManager;
    List<Sensor> listSensor;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_list_sensors);

        TextView tvOS = (TextView) findViewById(R.id.tvOS);
        TextView tvManufacturer = (TextView) findViewById(R.id.tvManufacturer);

        tvOS.setText("Android " + Build.VERSION.RELEASE);
        tvManufacturer.setText(Build.MANUFACTURER);

        sensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        listSensor = sensorManager.getSensorList(Sensor.TYPE_ALL);

        ArrayList<String> sensorNames = new ArrayList<String>();

        for (Sensor sensor : listSensor) {
            sensorNames.add(sensor.getName());
        }

        ListView lvSensors = (ListView) findViewById(R.id.lvSensors);
        lvSensors.setAdapter(new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, sensorNames));

        //ListView ItemClicked Handler

        lvSensors.setOnItemClickListener(new ListView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                //Display an info Dialog
                AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(ListSensorsActivity.this);

                Sensor sensor = listSensor.get(position);
                dialogBuilder.setTitle(sensor.getName());
                dialogBuilder.setMessage(
                        "Name: " + sensor.getName() +
                                "\n\nType: " + sensor.getStringType() +
                                "\n\nVendor: " + sensor.getVendor() +
                                "\n\nPower Usage: " + sensor.getPower() + " mA"
                );

                dialogBuilder.create().show();
            }
        });


    }
}
