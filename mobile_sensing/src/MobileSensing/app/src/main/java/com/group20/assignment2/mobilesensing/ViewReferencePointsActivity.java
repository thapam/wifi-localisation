package com.group20.assignment2.mobilesensing;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

public class ViewReferencePointsActivity extends ActionBarActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_view_reference_points);


        //Handler for Spinner References
        Spinner spinnerReference = (Spinner) findViewById(R.id.spinnerReference);
        spinnerReference.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {

            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {

                String refName = ((TextView) view).getText().toString();

                try {

                    LocationPoint referencePoint = new LocationPoint(refName);

                    ListView lvMeasurementDetails = (ListView) findViewById(R.id.lvWifiFingerprint);
                    lvMeasurementDetails.setAdapter(new customListAdapter(ViewReferencePointsActivity.this, referencePoint.fingerprint));

                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });


        //handler for DELETE button
        Button buttonDelete = (Button) findViewById(R.id.buttonDeleteReference);
        buttonDelete.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog.Builder builder = new AlertDialog.Builder(ViewReferencePointsActivity.this);
                builder.setTitle("Delete Reference?");
                builder.setMessage("Are you sure you want to delete this reference?");

                builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                    }
                });

                builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        Spinner spinnerReference = (Spinner) findViewById(R.id.spinnerReference);
                        String dirName = spinnerReference.getSelectedItem().toString();

                        File file = new File(MyApplication.getExtStoragePath() + "/reference_data/" + dirName);

                        File[] files = file.listFiles();

                        for (File f : files)
                            f.delete();

                        if (file.delete()) {
                            Toast.makeText(ViewReferencePointsActivity.this, "Reference has been deleted", Toast.LENGTH_SHORT).show();
                            ViewReferencePointsActivity.this.finish();
                            startActivity(getIntent());
                        } else {
                            Toast.makeText(ViewReferencePointsActivity.this, "Could not delete reference", Toast.LENGTH_SHORT).show();
                        }
                    }
                });

                builder.create().show();
            }
        });
    }


    protected void onResume() {

        super.onResume();
        String[] ref = new File(MyApplication.getExtStoragePath() + "/reference_data").list();

        ArrayList<String> references = new ArrayList<String>();

        for (String r : ref) {
            String[] files = (new File(MyApplication.getExtStoragePath() + "/reference_data/" + r)).list();
            if (files.length != 0)
                references.add(r);
        }

        Spinner spinnerReference = (Spinner) findViewById(R.id.spinnerReference);
        spinnerReference.setAdapter(new ArrayAdapter<String>(this, android.R.layout.simple_spinner_dropdown_item, references));

        String selection;
        selection = getIntent().getStringExtra("selection");

        if (selection != null) {
            for (int i = 0; i < references.size(); i++) {
                if (selection.equals(references.get(i))) {
                    spinnerReference.setSelection(i, true);
                }
            }
        }

    }
}



