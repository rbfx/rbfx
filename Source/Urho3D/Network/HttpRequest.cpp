//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include "../Core/Profiler.h"
#include "../Core/Timer.h"
#include "../IO/Log.h"
#include "../Network/HttpRequest.h"

#include <Civetweb/civetweb.h>

#include "../DebugNew.h"

namespace Urho3D
{

static const unsigned ERROR_BUFFER_SIZE = 256;
static const unsigned READ_BUFFER_SIZE = 65536; // Must be a power of two

HttpRequest::HttpRequest(const ea::string& url, const ea::string& verb, const ea::vector<ea::string>& headers, const ea::string& postData) :
    url_(url.trimmed()),
    verb_(!verb.empty() ? verb : "GET"),
    headers_(headers),
    postData_(postData),
    state_(HTTP_INITIALIZING),
    httpReadBuffer_(new unsigned char[READ_BUFFER_SIZE]),
    readBuffer_(new unsigned char[READ_BUFFER_SIZE]),
    readPosition_(0),
    writePosition_(0)
{
    // Size of response is unknown, so just set maximum value. The position will also be changed
    // to maximum value once the request is done, signaling end for Deserializer::IsEof().
    size_ = M_MAX_UNSIGNED;

    URHO3D_LOGDEBUG("HTTP " + verb_ + " request to URL " + url_);

#ifdef URHO3D_SSL
    static bool sslInitialized = false;
    if (!sslInitialized)
    {
        mg_init_library(MG_FEATURES_TLS);
        sslInitialized = true;
    }
#endif

#ifdef URHO3D_THREADING
    // Start the worker thread to actually create the connection and read the response data.
    Run();
#else
    URHO3D_LOGERROR("HTTP request will not execute as threading is disabled");
#endif
}

HttpRequest::~HttpRequest()
{
    Stop();
}

void HttpRequest::ThreadFunction()
{
    URHO3D_PROFILE_THREAD("HttpRequest Thread");

    ea::string protocol = "http";
    ea::string host;
    ea::string path = "/";
    int port = 80;

    unsigned protocolEnd = url_.find("://");
    if (protocolEnd != ea::string::npos)
    {
        protocol = url_.substr(0, protocolEnd);
        host = url_.substr(protocolEnd + 3);
    }
    else
        host = url_;

    unsigned pathStart = host.find('/');
    if (pathStart != ea::string::npos)
    {
        path = host.substr(pathStart);
        host = host.substr(0, pathStart);
    }

    unsigned portStart = host.find(':');
    if (portStart != ea::string::npos)
    {
        port = ToInt(host.substr(portStart + 1));
        host = host.substr(0, portStart);
    } else if (protocol.comparei("https") >= 0)
        port = 443;

    char errorBuffer[ERROR_BUFFER_SIZE];
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
        connection = mg_download(host.c_str(), port, protocol.comparei("https") >= 0 ? 1 : 0, errorBuffer, sizeof(errorBuffer),
            "%s %s HTTP/1.0\r\n"
            "Host: %s\r\n"
            "%s"
            "\r\n", verb_.c_str(), path.c_str(), host.c_str(), headersStr.c_str());
    }
    else
    {
        connection = mg_download(host.c_str(), port, protocol.comparei("https") >= 0 ? 1 : 0, errorBuffer, sizeof(errorBuffer),
            "%s %s HTTP/1.0\r\n"
            "Host: %s\r\n"
            "%s"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s", verb_.c_str(), path.c_str(), host.c_str(), headersStr.c_str(), postData_.length(), postData_.c_str());
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
        int bytesRead = mg_read(connection, httpReadBuffer_.get(), READ_BUFFER_SIZE / 4);
        if (bytesRead <= 0)
            break;

        mutex_.Acquire();

        // Wait until enough space in the main thread's ring buffer
        for (;;)
        {
            unsigned spaceInBuffer = READ_BUFFER_SIZE - ((writePosition_ - readPosition_) & (READ_BUFFER_SIZE - 1));
            if ((int)spaceInBuffer > bytesRead || !shouldRun_)
                break;

            mutex_.Release();
            Time::Sleep(5);
            mutex_.Acquire();
        }

        if (!shouldRun_)
        {
            mutex_.Release();
            break;
        }

        if (writePosition_ + bytesRead <= READ_BUFFER_SIZE)
            memcpy(readBuffer_.get() + writePosition_, httpReadBuffer_.get(), (size_t)bytesRead);
        else
        {
            // Handle ring buffer wrap
            unsigned part1 = READ_BUFFER_SIZE - writePosition_;
            unsigned part2 = bytesRead - part1;
            memcpy(readBuffer_.get() + writePosition_, httpReadBuffer_.get(), part1);
            memcpy(readBuffer_.get(), httpReadBuffer_.get() + part1, part2);
        }

        writePosition_ += bytesRead;
        writePosition_ &= READ_BUFFER_SIZE - 1;

        mutex_.Release();
    }

    // Close the connection
    mg_close_connection(connection);

    {
        MutexLock lock(mutex_);
        state_ = HTTP_CLOSED;
    }
}

unsigned HttpRequest::Read(void* dest, unsigned size)
{
#ifdef URHO3D_THREADING
    mutex_.Acquire();

    auto* destPtr = (unsigned char*)dest;
    unsigned sizeLeft = size;
    unsigned totalRead = 0;

    for (;;)
    {
        ea::pair<unsigned, bool> status{};

        for (;;)
        {
            status = CheckAvailableSizeAndEof();
            if (status.first || status.second)
                break;
            // While no bytes and connection is still open, block until has some data
            mutex_.Release();
            Time::Sleep(5);
            mutex_.Acquire();
        }

        unsigned bytesAvailable = status.first;

        if (bytesAvailable)
        {
            if (bytesAvailable > sizeLeft)
                bytesAvailable = sizeLeft;

            if (readPosition_ + bytesAvailable <= READ_BUFFER_SIZE)
                memcpy(destPtr, readBuffer_.get() + readPosition_, bytesAvailable);
            else
            {
                // Handle ring buffer wrap
                unsigned part1 = READ_BUFFER_SIZE - readPosition_;
                unsigned part2 = bytesAvailable - part1;
                memcpy(destPtr, readBuffer_.get() + readPosition_, part1);
                memcpy(destPtr + part1, readBuffer_.get(), part2);
            }

            readPosition_ += bytesAvailable;
            readPosition_ &= READ_BUFFER_SIZE - 1;
            sizeLeft -= bytesAvailable;
            totalRead += bytesAvailable;
            destPtr += bytesAvailable;
        }

        if (!sizeLeft || !bytesAvailable)
            break;
    }

    mutex_.Release();
    return totalRead;
#else
    // Threading disabled, nothing to read
    return 0;
#endif
}

unsigned HttpRequest::Seek(unsigned position)
{
    return 0;
}

bool HttpRequest::IsEof() const
{
    MutexLock lock(mutex_);
    return CheckAvailableSizeAndEof().second;
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
    return CheckAvailableSizeAndEof().first;
}

ea::pair<unsigned, bool> HttpRequest::CheckAvailableSizeAndEof() const
{
    unsigned size = (writePosition_ - readPosition_) & (READ_BUFFER_SIZE - 1);
    return {size, (state_ == HTTP_ERROR || (state_ == HTTP_CLOSED && !size))};
}

}
