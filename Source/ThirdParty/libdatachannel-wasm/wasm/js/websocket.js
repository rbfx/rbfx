/**
 * Copyright (c) 2017-2022 Paul-Louis Ageneau
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

(function() {
	var WebSocket = {
		$WEBSOCKET: {
			map: {},
			nextId: 1,

			allocUTF8FromString: function(str) {
				var strLen = lengthBytesUTF8(str);
				var strOnHeap = _malloc(strLen+1);
				stringToUTF8(str, strOnHeap, strLen+1);
				return strOnHeap;
			},

			registerWebSocket: function(webSocket) {
				var ws = WEBSOCKET.nextId++;
				WEBSOCKET.map[ws] = webSocket;
				webSocket.binaryType = 'arraybuffer';
				return ws;
			},
		},

		wsCreateWebSocket: function(pUrl) {
			var url = UTF8ToString(pUrl);
			if(!window.WebSocket) return 0;
			return WEBSOCKET.registerWebSocket(new WebSocket(url));
		},

		wsDeleteWebSocket: function(ws) {
			var webSocket = WEBSOCKET.map[ws];
			if(webSocket) {
				webSocket.close();
				webSocket.wsUserDeleted = true;
				delete WEBSOCKET.map[ws];
			}
		},

		wsSetOpenCallback: function(ws, openCallback) {
			var webSocket = WEBSOCKET.map[ws];
			var cb = function() {
				if(webSocket.rtcUserDeleted) return;
				var userPointer = webSocket.rtcUserPointer || 0;
				{{{ makeDynCall('vi', 'openCallback') }}} (userPointer);
			};
			webSocket.onopen = cb;
			if(webSocket.readyState == 1) setTimeout(cb, 0);
		},

 		wsSetErrorCallback: function(ws, errorCallback) {
			var webSocket = WEBSOCKET.map[ws];
			var cb = function() {
				if(webSocket.rtcUserDeleted) return;
				var userPointer = webSocket.rtcUserPointer || 0;
        {{{ makeDynCall('vii', 'errorCallback') }}} (0, userPointer);
			};
			webSocket.onerror = cb;
		},

		wsSetMessageCallback: function(ws, messageCallback) {
			var webSocket = WEBSOCKET.map[ws];
			webSocket.onmessage = function(evt) {
				if(webSocket.rtcUserDeleted) return;
				if(typeof evt.data == 'string') {
					var pStr = WEBSOCKET.allocUTF8FromString(evt.data);
					var userPointer = webSocket.rtcUserPointer || 0;
					{{{ makeDynCall('viii', 'messageCallback') }}} (pStr, -1, userPointer);
					_free(pStr);
				} else {
					var byteArray = new Uint8Array(evt.data);
					var size = byteArray.byteLength;
					var pBuffer = _malloc(size);
					var heapBytes = new Uint8Array(Module['HEAPU8'].buffer, pBuffer, size);
					heapBytes.set(byteArray);
					var userPointer = webSocket.rtcUserPointer || 0;
					{{{ makeDynCall('viii', 'messageCallback') }}} (pBuffer, size, userPointer);
					_free(pBuffer);
				}
			};
			webSocket.onclose = function() {
				if(webSocket.rtcUserDeleted) return;
				var userPointer = webSocket.rtcUserPointer || 0;
				{{{ makeDynCall('viii', 'messageCallback') }}} (0, 0, userPointer);
			};
		},

		wsSendMessage: function(ws, pBuffer, size) {
			var webSocket = WEBSOCKET.map[ws];
			if(webSocket.readyState != 1) return -1;
			if(size >= 0) {
				var heapBytes = new Uint8Array(Module['HEAPU8'].buffer, pBuffer, size);
				webSocket.send(heapBytes);
				return size;
			} else {
				var str = UTF8ToString(pBuffer);
				webSocket.send(str);
				return lengthBytesUTF8(str);
			}
		},

		wsSetUserPointer: function(ws, ptr) {
			var webSocket = WEBSOCKET.map[ws];
			if(webSocket) webSocket.rtcUserPointer = ptr;
		},
	};

	autoAddDeps(WebSocket, '$WEBSOCKET');
	mergeInto(LibraryManager.library, WebSocket);
})();

