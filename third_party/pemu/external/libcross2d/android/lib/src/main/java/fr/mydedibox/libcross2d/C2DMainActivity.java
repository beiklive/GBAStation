package fr.mydedibox.libcross2d;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

public class C2DMainActivity extends AppCompatActivity {
    final private int REQUEST_CODE_ASK_MULTIPLE_PERMISSIONS = 124;
    boolean checkPermissions = false;
    String dataPath;
    String romFsPath;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        checkRuntimePermissions();
    }

    protected String getDataName() {
        return "c2d";
    }

    protected String[] getLibraries() {
        return new String[]{
                ""
        };
    }

    private String getApplicationName() {
        ApplicationInfo applicationInfo = getApplicationInfo();
        int stringId = applicationInfo.labelRes;
        return stringId == 0 ? applicationInfo.nonLocalizedLabel.toString() : getString(stringId);
    }

    private void start() {
        Log.v("C2D", "start");

        boolean external = Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED);
        boolean canWriteExternal = external && checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
        if (canWriteExternal) {
            // TODO: is this really needed
            File f = new File(Environment.getExternalStorageDirectory().getAbsolutePath() + "/.pemutest");
            f.delete();
            canWriteExternal = f.mkdirs();
            f.delete();
        }

        dataPath = canWriteExternal ? Environment.getExternalStorageDirectory().getAbsolutePath() + "/Pemu/" + getApplicationName()
                : getFilesDir().getAbsolutePath();

        romFsPath = external ? getExternalFilesDir(null).getAbsolutePath() : getFilesDir().getAbsolutePath();

        Log.v("C2D", "data: " + dataPath + " (external: " + external + ", canWriteExternal: " + canWriteExternal + ")");
        Log.v("C2D", "romfs: " + romFsPath + "/romfs (external: " + external + ", canWriteExternal: " + canWriteExternal + ")");

        // extract libcross2d "romfs"
        if (!isDataUpToDate("romfs", romFsPath)) {
            copyAssets("romfs", romFsPath);
        }

        // extract libcross2d "data"
        if (!isDataUpToDate("data", dataPath)) {
            copyAssets("data", dataPath);
        }

        // load c2d sdl2 wrapper activity
        Intent c2d = new Intent(this, C2DSDLActivity.class);
        c2d.putExtra("DATA", dataPath);
        c2d.putExtra("ROMFS", romFsPath + "/romfs");
        c2d.putExtra("LIBS", getLibraries());
        startActivity(c2d);
        finish();
    }

    //region assets
    public boolean isDataUpToDate(String assetPath, String dataPath) {
        String storageVersion, assetVersion;

        // get data version
        try {
            byte[] bytes = Files.readAllBytes(Paths.get(dataPath + "/" + assetPath + "/version"));
            storageVersion = new String(bytes).trim();
        } catch (IOException e) {
            return false;
        }

        // get asset version
        try {
            InputStream is = getAssets().open(assetPath + "/version");
            BufferedReader br = new BufferedReader(new InputStreamReader(is, StandardCharsets.UTF_8));
            assetVersion = br.readLine().trim();
            br.close();
        } catch (IOException e) {
            return false;
        }

        Log.v("C2D", dataPath + "/" + assetPath + ": asset version: "
                + assetVersion + ", storage version: " + storageVersion);

        return assetVersion.equals(storageVersion);
    }

    public boolean copyAssets(String assetPath, String dataPath) {
        Log.v("C2D", "copyAssets: " + assetPath + " => " + dataPath);
        try {
            String[] list = getAssets().list(assetPath);
            if (list.length > 0) {
                create(dataPath + "/" + assetPath);
                for (String file : list) {
                    if (!copyAssets(assetPath + "/" + file, dataPath)) {
                        return false;
                    } else {
                        String src = assetPath + "/" + file;
                        String dst = dataPath + "/" + src;
                        if (isFile(src)) {
                            copy(src, dst);
                        }
                    }
                }
            }
        } catch (IOException e) {
            return false;
        }

        return true;
    }

    private boolean isFile(String asset) {
        try {
            InputStream in = getAssets().open(asset);
            in.close();
        } catch (IOException e) {
            return false;
        }

        return true;
    }

    private void create(String path) {
        File dir = new File(path);
        if (!dir.exists()) {
            Log.v("C2D", "create: " + dir.getAbsolutePath());
            dir.mkdirs();
        }
    }

    private void copy(String asset, String dst) throws IOException {
        Log.v("C2D", "copy: " + asset + " => " + dst);
        InputStream in = null;
        OutputStream out = null;

        try {
            byte[] buffer = new byte[1024 * 4];
            int read;
            in = getAssets().open(asset);
            out = new FileOutputStream(dst);
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
        } catch (IOException e) {
            Log.e("C2D", "copy failed: " + dst);
        } finally {
            if (in != null) {
                try {
                    in.close();
                } catch (IOException e) {
                    // NOOP
                }
            }
            if (out != null) {
                try {
                    out.close();
                } catch (IOException e) {
                    // NOOP
                }
            }
        }
    }
    //endregion

    //region permissions
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        Log.e("C2D", "onRequestPermissionsResult");
        if (requestCode == REQUEST_CODE_ASK_MULTIPLE_PERMISSIONS) {
            for (int i = 0; i < permissions.length; i++) {
                if (grantResults[i] == PackageManager.PERMISSION_GRANTED) {
                    Log.i("C2D", "Permission: " + permissions[i] + " was granted.");
                } else {
                    Log.i("C2D", "Permission: " + permissions[i] + " was not granted.");
                }
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }

        start();
    }

    @SuppressLint("ObsoleteSdkInt")
    public void checkRuntimePermissions() {
        Log.e("C2D", "checkRuntimePermissions");
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            // Android 6.0+ needs runtime permission checks
            List<String> permissionsNeeded = new ArrayList<>();
            final List<String> permissionsList = new ArrayList<>();
            if (!addPermission(permissionsList, Manifest.permission.READ_EXTERNAL_STORAGE))
                permissionsNeeded.add("Read External Storage");
            if (!addPermission(permissionsList, Manifest.permission.WRITE_EXTERNAL_STORAGE))
                permissionsNeeded.add("Write External Storage");
            if (permissionsList.size() > 0) {
                checkPermissions = true;
                if (permissionsNeeded.size() > 0) {
                    // Need Rationale
                    Log.i("C2D", "Need to request external storage permissions.");

                    StringBuilder message = new StringBuilder("You need to grant access to " + permissionsNeeded.get(0));
                    for (int i = 1; i < permissionsNeeded.size(); i++) {
                        message.append(", ").append(permissionsNeeded.get(i));
                    }

                    showMessageOKCancel(message.toString(), (dialog, which) -> {
                        if (which == AlertDialog.BUTTON_POSITIVE) {
                            requestPermissions(permissionsList.toArray(new String[permissionsList.size()]),
                                    REQUEST_CODE_ASK_MULTIPLE_PERMISSIONS);
                            Log.i("C2D", "User accepted request for external storage permissions.");
                        } else {
                            Log.i("C2D", "User declined request for external storage permissions.");
                            start();
                        }
                    });
                } else {
                    requestPermissions(permissionsList.toArray(new String[permissionsList.size()]),
                            REQUEST_CODE_ASK_MULTIPLE_PERMISSIONS);

                    Log.i("C2D", "Requested external storage permissions.");
                }
            }
        }

        if (!checkPermissions) {
            start();
        }
    }

    public void showMessageOKCancel(String message, DialogInterface.OnClickListener onClickListener) {
        new AlertDialog.Builder(this).setMessage(message)
                .setPositiveButton("OK", onClickListener).setCancelable(false)
                .setNegativeButton("Cancel", onClickListener).create().show();
    }

    @SuppressLint("ObsoleteSdkInt")
    private boolean addPermission(List<String> permissionsList, String permission) {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
                permissionsList.add(permission);
                return shouldShowRequestPermissionRationale(permission);
            }
        }
        return true;
    }
    //endregion
}