package com.beiklive.gbastation;

import android.os.Bundle;

import org.libsdl.app.SDLActivity;

public class GBAStationActivity extends SDLActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        System.exit(0);
    }

    @Override
    protected String[] getLibraries() {
        return new String[] {
            "SDL3",
            "GBAStation"
        };
    }
}
