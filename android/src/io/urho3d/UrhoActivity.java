//
// Copyright (c) 2008-2018 the Urho3D project.
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

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
            return libraryNames;
        }
    }
}
