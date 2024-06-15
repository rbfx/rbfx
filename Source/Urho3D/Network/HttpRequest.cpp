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
#include <emscripten/fetch.h>
#else
#include <Civetweb/civetweb.h>
#endif

#include "../DebugNew.h"

namespace Urho3D
{

static const unsigned ERROR_BUFFER_SIZE = 256;
static const unsigned READ_BUFFER_SIZE = 1024;

#if defined(URHO3D_PLATFORM_WEB)
void LogFetch(const ea::string& ctx, emscripten_fetch_t* fetch)
{
    URHO3D_LOGDEBUG(
        "{} "
        "readyState={} "
        "status={} "
        "statusText={} "
        "totalBytes={} "
        "dataOffset={} "
        "numBytes={} "
        "data={} "
        "url={} "
        "userData={} "
        "id={}",
        ctx,
        fetch->readyState,
        fetch->status,
        &fetch->statusText[0],
        fetch->totalBytes,
        fetch->dataOffset,
        fetch->numBytes,
        fetch->data ? "true" : "false",
        fetch->url ? fetch->url : "",
        fetch->userData ? "true" : "false",
        fetch->id);
}
#endif

HttpRequest::HttpRequest(
    const ea::string& url, const ea::string& verb, const ea::vector<ea::string>& headers, const ea::string& postData)
    : url_(url.trimmed())
    , verb_(!verb.empty() ? verb : "GET")
    , headers_(headers)
    , postData_(postData)
{
    // Size of response is unknown, so just set maximum value. The position will also be changed
    // to maximum value once the request is done, signaling end for Deserializer::IsEof().
    size_ = M_MAX_UNSIGNED;
    URHO3D_LOGDEBUG("HTTP {} request to URL {} {} {}", verb_, url_.ToString(), ea::string::joined(headers, ","), postData);
#if defined(URHO3D_PLATFORM_WEB)
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, verb_.c_str());
    attr.requestData = postData_.c_str();
    attr.requestDataSize = postData_.size();
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.userData = this;

    attr.onsuccess = [](emscripten_fetch_t * fetch)
    {
        LogFetch("HTTP OnFetchSucceeded", fetch);

        HttpRequest* request = static_cast<HttpRequest*>(fetch->userData);
        MutexLock lock(request->mutex_);
        request->state_ = HTTP_CLOSED;
        request->readBuffer_.Resize(fetch->numBytes - 1);
        request->readBuffer_.SetData(fetch->data, fetch->numBytes - 1);
        request->requestHandle_ = nullptr;

        emscripten_fetch_close(fetch);
    };

    attr.onerror = [](emscripten_fetch_t* fetch)
    {
        LogFetch("HTTP OnFetchError", fetch);

        HttpRequest* request = static_cast<HttpRequest*>(fetch->userData);
        MutexLock lock(request->mutex_);
        request->state_ = HTTP_ERROR;
        request->error_ = fetch->statusText;
        request->requestHandle_ = nullptr;

        emscripten_fetch_close(fetch);
    };

    attr.onprogress = [](emscripten_fetch_t * fetch)
    {
        LogFetch("HTTP OnFetchProgress", fetch);
    };

    attr.onreadystatechange = [](emscripten_fetch_t * fetch)
    {
        LogFetch("HTTP OnFetchReadyStateChange", fetch);
    };

    if (!headers_.empty())
    {
        for (const auto& header : headers_)
        {
            const auto pair = header.trimmed().split(':');

            const auto expectedSize = 2;
            if (pair.size() != expectedSize)
            {
                URHO3D_LOGWARNING("HTTP ignoring header '{}' with unexpected format, expected format 'key: value'", header);
                continue;
            }

            const auto keyIndex = 0;
            const auto& key = pair[keyIndex].trimmed();
            if (key.empty())
            {
                URHO3D_LOGWARNING("HTTP ignoring header '{}' with empty key", header);
                continue;
            }

            const auto valueIndex = 1;
            const auto& value = pair[valueIndex].trimmed();
            if (value.empty())
            {
                URHO3D_LOGWARNING("HTTP ignoring header '{}' with empty value", header);
                continue;
            }

            requestHeadersStr_.push_back(key);
            requestHeadersStr_.push_back(value);
        }

        if (!requestHeadersStr_.empty())
        {
            for (const auto& rh : requestHeadersStr_)
            {
                requestHeaders_.push_back(rh.c_str());
            }
            requestHeaders_.push_back(nullptr);
            attr.requestHeaders = requestHeaders_.data();
        }
    }

    emscripten_fetch(&attr, url.c_str());

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
        emscripten_fetch_close((emscripten_fetch_t*)requestHandle_);
        requestHandle_ = nullptr;
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
