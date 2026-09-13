// Copyright (c) 2008-2018 the Urho3D project.
// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

package io.urho3d;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.util.*;

public class UrhoActivity extends SDLActivity {

    private static final String TAG = "Urho3D";
    private static String[] mArguments = new String[0];

    @Override
    protected String[] getArguments() {
        return mArguments;
    }

    @Override
    public void onBackPressed() {
        finish();
    }

    public static ArrayList<String> getLibraryNames(SDLActivity activity)
    {
        File[] files = new File(activity.getApplicationInfo().nativeLibraryDir).listFiles((dir, filename) -> {
            // Only list libraries, i.e. exclude gdbserver when it presents
            return filename.matches("^lib.*\\.so$");
        });
        if (files == null) {
            return null;
        } else {
            Arrays.sort(files, (lhs, rhs) -> Long.valueOf(lhs.lastModified()).compareTo(rhs.lastModified()));
            ArrayList<String> libraryNames = new ArrayList<>(files.length);
            for (final File libraryFilename : files) {
                libraryNames.add(libraryFilename.getName().replaceAll("^lib(.*)\\.so$", "$1"));
            }

            // Load engine first and player last
            int index = libraryNames.indexOf("Urho3D");
            if (index >= 0) {
                // Static builds would not contain this library.
                libraryNames.add(0, libraryNames.remove(index));
            }
            index = libraryNames.indexOf("Player");
            if (index >= 0) {
                // Static builds would not contain this library.
                libraryNames.add(libraryNames.size() - 1, libraryNames.remove(index));
            }
            index = libraryNames.indexOf("c++_shared");
            if (index >= 0) {
                // Static builds would not contain this library.
                libraryNames.add(0, libraryNames.remove(index));
            }
            index = libraryNames.indexOf("openxr_loader");
            if (index >= 0) {
                // Non-XR builds would not contain this library.
                libraryNames.add(0, libraryNames.remove(index));
            }
            return libraryNames;
        }
    }
}
