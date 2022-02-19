//
// Copyright (c) 2008-2022 the Urho3D project.
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

/// \file

#pragma once

#include <EASTL/shared_array.h>

#include "../Core/Mutex.h"
#include "../Container/RefCounted.h"
#include "../Core/Thread.h"
#include "../IO/Deserializer.h"

namespace Urho3D
{

/// HTTP connection state.
enum HttpRequestState
{
    HTTP_INITIALIZING = 0,
    HTTP_ERROR,
    HTTP_OPEN,
    HTTP_CLOSED
};

/// An HTTP connection with response data stream.
class URHO3D_API HttpRequest : public RefCounted, public Deserializer, public Thread
{
public:
    /// Construct with parameters.
    HttpRequest(const ea::string& url, const ea::string& verb, const ea::vector<ea::string>& headers, const ea::string& postData);
    /// Destruct. Release the connection object.
    ~HttpRequest() override;

    /// Process the connection in the worker thread until closed.
    void ThreadFunction() override;

    /// Read response data from the HTTP connection and return number of bytes actually read. While the connection is open, will block while trying to read the specified size. To avoid blocking, only read up to as many bytes as GetAvailableSize() returns.
    unsigned Read(void* dest, unsigned size) override;
    /// Set position from the beginning of the stream. Not supported.
    unsigned Seek(unsigned position) override;
    /// Return whether all response data has been read.
    bool IsEof() const override;

    /// Return URL used in the request.
    /// @property{get_url}
    const ea::string& GetURL() const { return url_; }

    /// Return verb used in the request. Default GET if empty verb specified on construction.
    /// @property
    const ea::string& GetVerb() const { return verb_; }

    /// Return error. Only non-empty in the error state.
    /// @property
    ea::string GetError() const;
    /// Return connection state.
    /// @property
    HttpRequestState GetState() const;
    /// Return amount of bytes in the read buffer.
    /// @property
    unsigned GetAvailableSize() const;

    /// Return whether connection is in the open state.
    /// @property
    bool IsOpen() const { return GetState() == HTTP_OPEN; }

private:
    /// Check for available read data in buffer and whether end has been reached. Must only be called when the mutex is held by the main thread.
    ea::pair<unsigned, bool> CheckAvailableSizeAndEof() const;

    /// URL.
    ea::string url_;
    /// Verb.
    ea::string verb_;
    /// Error string. Empty if no error.
    ea::string error_;
    /// Headers.
    ea::vector<ea::string> headers_;
    /// POST data.
    ea::string postData_;
    /// Connection state.
    HttpRequestState state_;
    /// Mutex for synchronizing the worker and the main thread.
    mutable Mutex mutex_;
    /// Read buffer for the worker thread.
    ea::shared_array<unsigned char> httpReadBuffer_;
    /// Read buffer for the main thread.
    ea::shared_array<unsigned char> readBuffer_;
    /// Read buffer read cursor.
    unsigned readPosition_;
    /// Read buffer write cursor.
    unsigned writePosition_;
};

}
