//
// Copyright (c) 2023-2023 the rbfx project.
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

package io.rebelfork;

import java.util.*;

// Helper class for BillingClient.
public class RbfxLambdaContainer
{
    // Lambda container constructor. Executed from native code with a pointer to lambda container native object as argument.
    public RbfxLambdaContainer(long ptr)
    {
        lambdaPtr_ = ptr;
    }

    // Finalizer to destroy the lambda container native object.
    @Override
    protected void finalize() throws Throwable {
        try {
            disposePtr(lambdaPtr_);
        } finally {
            super.finalize();
        }
    }

    // Native method to destroy native lambda container object.
    private native void disposePtr(long ptr);


    // Pointer to a native abstract class.
    private long lambdaPtr_;
}
