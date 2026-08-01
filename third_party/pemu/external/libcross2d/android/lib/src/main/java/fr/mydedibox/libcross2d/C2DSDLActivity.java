package fr.mydedibox.libcross2d;

import android.content.Intent;
import android.os.Build;
import android.os.Bundle;

import org.libsdl.app.SDLActivity;

public class C2DSDLActivity extends SDLActivity {
    String[] libraries;
    public static String dataPath;
    public static String romFsPath;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // parse arguments
        Intent c2d = getIntent();
        libraries = c2d.getStringArrayExtra("LIBS");
        dataPath = c2d.getStringExtra("DATA");
        romFsPath = c2d.getStringExtra("ROMFS");

        super.onCreate(savedInstanceState);
    }

    @Override
    protected String[] getLibraries() {
        return libraries;
    }

    //region jni
    public static String getDataPath() {
        return dataPath;
    }

    public static String getRomFsPath() {
        return romFsPath;
    }

    public static String getDeviceName() {
        return Build.DEVICE;
    }
    //endregion
}
