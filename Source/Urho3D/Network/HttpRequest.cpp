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

#include "../Precompiled.h"

#include <Urho3D/Core/Profiler.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Network/HttpRequest.h>

#ifdef URHO3D_PLATFORM_WEB
#include <emscripten.h>
#else
#include <Civetweb/civetweb.h>
#endif

#include "../DebugNew.h"

namespace Urho3D
{

static const unsigned ERROR_BUFFER_SIZE = 256;
static const unsigned READ_BUFFER_SIZE = 1024;

HttpRequest::HttpRequest(const ea::string& url, const ea::string& verb, const ea::vector<ea::string>& headers, const ea::string& postData) :
    url_(url.trimmed()),
    verb_(!verb.empty() ? verb : "GET"),
    headers_(headers),
    postData_(postData)
{
    // Size of response is unknown, so just set maximum value. The position will also be changed
    // to maximum value once the request is done, signaling end for Deserializer::IsEof().
    size_ = M_MAX_UNSIGNED;
    URHO3D_LOGDEBUG("HTTP {} request to URL {}", verb_, url_.ToString());

#if defined(URHO3D_PLATFORM_WEB)
    if (!headers.empty())
        Log::GetLogger("HttpRequest").Warning("Custom headers in Web builds are not supported and were ignored!");

    requestHandle_ = emscripten_async_wget2_data(url_.ToString().c_str(), verb_.c_str(), postData.c_str(), this, 1,
        [](unsigned handle, void* arg, void* buf, unsigned len)         // onload
        {
            HttpRequest* request = static_cast<HttpRequest*>(arg);
            MutexLock lock(request->mutex_);
            request->state_ = HTTP_CLOSED;
            request->readBuffer_.Resize(len);
            request->readBuffer_.SetData(buf, len);
            request->requestHandle_ = 0;
        }, [](unsigned handle, void* arg, int code, const char* msg)    // onerror
        {
            HttpRequest* request = static_cast<HttpRequest*>(arg);
            MutexLock lock(request->mutex_);
            request->state_ = HTTP_ERROR;
            request->error_ = msg;
            request->requestHandle_ = 0;
        }, [](unsigned handle, void* arg, int loaded, int total)        // onprogress
        {
            //HttpRequest* request = static_cast<HttpRequest*>(arg);
        });
    if (requestHandle_)
        state_ = HTTP_OPEN;
#elif defined(URHO3D_THREADING)
    static bool sslInitialized = false;
    if (!sslInitialized)
    {
        mg_init_library(MG_FEATURES_TLS);
        sslInitialized = true;
    }
    // Start the worker thread to actually create the connection and read the response data.
    Run();
#else
    URHO3D_LOGERROR("HTTP request will not execute as threading is disabled");
#endif  // URHO3D_PLATFORM_WEB
}

HttpRequest::~HttpRequest()
{
#ifdef URHO3D_PLATFORM_WEB
    if (requestHandle_)
    {
        emscripten_async_wget2_abort(requestHandle_);
        requestHandle_ = 0;
    }
#endif
    Stop();
}

void HttpRequest::ThreadFunction()
{
#ifndef URHO3D_PLATFORM_WEB
    URHO3D_PROFILE_THREAD("HttpRequest Thread");

    char errorBuffer[ERROR_BUFFER_SIZE];
    char readBuffer[READ_BUFFER_SIZE];
    memset(errorBuffer, 0, sizeof(errorBuffer));

    ea::string headersStr;
    for (unsigned i = 0; i < headers_.size(); ++i)
    {
        // Trim and only add non-empty header strings
        ea::string header = headers_[i].trimmed();
        if (header.length())
            headersStr += header + "\r\n";
    }

    // Initiate the connection. This may block due to DNS query
    mg_connection* connection = nullptr;

    if (postData_.empty())
    {
        connection = mg_download(url_.host_.c_str(), url_.port_, url_.scheme_.comparei("https") >= 0 ? 1 : 0, errorBuffer, sizeof(errorBuffer),
            "%s %s HTTP/1.0\r\n"
            "Host: %s\r\n"
            "%s"
            "\r\n", verb_.c_str(), url_.path_.c_str(), url_.host_.c_str(), headersStr.c_str());
    }
    else
    {
        connection = mg_download(url_.host_.c_str(), url_.port_, url_.scheme_.comparei("https") >= 0 ? 1 : 0, errorBuffer, sizeof(errorBuffer),
            "%s %s HTTP/1.0\r\n"
            "Host: %s\r\n"
            "%s"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s", verb_.c_str(), url_.path_.c_str(), url_.host_.c_str(), headersStr.c_str(), postData_.length(), postData_.c_str());
    }

    {
        MutexLock lock(mutex_);
        state_ = connection ? HTTP_OPEN : HTTP_ERROR;

        // If no connection could be made, store the error and exit
        if (state_ == HTTP_ERROR)
        {
            error_ = ea::string(&errorBuffer[0]);
            return;
        }
    }

    // Loop while should run, read data from the connection, copy to the main thread buffer if there is space
    while (shouldRun_)
    {
        // Read less than full buffer to be able to distinguish between full and empty ring buffer. Reading may block
        int bytesRead = mg_read(connection, readBuffer, sizeof(readBuffer));
        if (bytesRead <= 0)
            break;

        MutexLock lock(mutex_);
        unsigned writePos = readBuffer_.GetSize();
        readBuffer_.Resize(writePos + bytesRead);
        memcpy(readBuffer_.GetModifiableData() + writePos, readBuffer, bytesRead);
    }

    // Close the connection
    mg_close_connection(connection);
    {
        MutexLock lock(mutex_);
        state_ = HTTP_CLOSED;
    }
#endif  // URHO3D_PLATFORM_WEB
}

unsigned HttpRequest::Read(void* dest, unsigned size)
{
    MutexLock lock(mutex_);
    unsigned readSize = ea::min(readBuffer_.GetSize(), size);
    memcpy(dest, readBuffer_.GetData() + readPosition_, readSize);
    readPosition_ += readSize;
    return readPosition_;
}

unsigned HttpRequest::Seek(unsigned position)
{
    return 0;
}

bool HttpRequest::IsEof() const
{
    MutexLock lock(mutex_);
    return GetAvailableSize() == 0;
}

ea::string HttpRequest::GetError() const
{
    MutexLock lock(mutex_);
    return error_;
}

HttpRequestState HttpRequest::GetState() const
{
    MutexLock lock(mutex_);
    return state_;
}

unsigned HttpRequest::GetAvailableSize() const
{
    MutexLock lock(mutex_);
    return readBuffer_.GetSize() - readPosition_;
}

}
