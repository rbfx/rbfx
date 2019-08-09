//
// Copyright (c) 2008-2018 the Urho3D project.
// Copyright (c) 2017-2019 the rbfx project.
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

package com.github.urho3d;

import org.libsdl.app.SDLActivity;

import java.util.*;

public class Samples extends SDLActivity {

    private static final String TAG = "Urho3D";
    private static String[] mArguments = new String[0];

    @Override
    protected String[] getArguments() {
        return mArguments;
    }

    @Override
    protected boolean onLoadLibrary(ArrayList<String> libraryNames) {
        // Ensure "Urho3D" shared library (if any) is being sorted to the top of the list
        // Also ensure STL runtime shared library (if any) is sorted to the top most entry
        Collections.sort(libraryNames, new Comparator<String>() {
            private String sortName(String name) {
                return name.matches("^\\d+_.+$") ? name : (name.matches("^.+_shared$") ? "0000_" : "000_") + name;
            }

            @Override
            public int compare(String lhs, String rhs) {
                return sortName(lhs).compareTo(sortName(rhs));
            }
        });
        return super.onLoadLibrary(libraryNames);
    }

    protected String getMainSharedObject() {
        return "libSamples.so";
    }

    @Override
    public void onBackPressed() {
        finish();
    }
}
