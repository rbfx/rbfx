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
	var WebRTC = {
		$WEBRTC: {
			peerConnectionsMap: {},
			dataChannelsMap: {},
			nextId: 1,

			allocUTF8FromString: function(str) {
				var strLen = lengthBytesUTF8(str);
				var strOnHeap = _malloc(strLen+1);
				stringToUTF8(str, strOnHeap, strLen+1);
				return strOnHeap;
			},

			registerPeerConnection: function(peerConnection) {
				var pc = WEBRTC.nextId++;
				WEBRTC.peerConnectionsMap[pc] = peerConnection;
				peerConnection.onnegotiationneeded = function() {
					peerConnection.createOffer()
						.then(function(offer) {
							return WEBRTC.handleDescription(peerConnection, offer);
						})
						.catch(function(err) {
							console.error(err);
						});
				};
				peerConnection.onicecandidate = function(evt) {
					if(evt.candidate && evt.candidate.candidate)
					  WEBRTC.handleCandidate(peerConnection, evt.candidate);
				};
				peerConnection.onconnectionstatechange = function() {
					WEBRTC.handleConnectionStateChange(peerConnection, peerConnection.connectionState)
				};
				peerConnection.onicegatheringstatechange = function() {
					WEBRTC.handleGatheringStateChange(peerConnection, peerConnection.iceGatheringState)
				};
				peerConnection.onsignalingstatechange = function() {
					WEBRTC.handleSignalingStateChange(peerConnection, peerConnection.signalingState)
				};
				return pc;
			},

			registerDataChannel: function(dataChannel) {
				var dc = WEBRTC.nextId++;
				WEBRTC.dataChannelsMap[dc] = dataChannel;
				dataChannel.binaryType = 'arraybuffer';
				return dc;
			},

			handleDescription: function(peerConnection, description) {
				return peerConnection.setLocalDescription(description)
					.then(function() {
						if(peerConnection.rtcUserDeleted) return;
						if(!peerConnection.rtcDescriptionCallback) return;
						var desc = peerConnection.localDescription;
						var pSdp = WEBRTC.allocUTF8FromString(desc.sdp);
						var pType = WEBRTC.allocUTF8FromString(desc.type);
						var callback =  peerConnection.rtcDescriptionCallback;
						var userPointer = peerConnection.rtcUserPointer || 0;
						{{{ makeDynCall('viii', 'callback') }}} (pSdp, pType, userPointer);
						_free(pSdp);
						_free(pType);
					});
			},

			handleCandidate: function(peerConnection, candidate) {
				if(peerConnection.rtcUserDeleted) return;
				if(!peerConnection.rtcCandidateCallback) return;
				var pCandidate = WEBRTC.allocUTF8FromString(candidate.candidate);
				var pSdpMid = WEBRTC.allocUTF8FromString(candidate.sdpMid);
				var candidateCallback =  peerConnection.rtcCandidateCallback;
				var userPointer = peerConnection.rtcUserPointer || 0;
				{{{ makeDynCall('viii', 'candidateCallback') }}} (pCandidate, pSdpMid, userPointer);
				_free(pCandidate);
				_free(pSdpMid);
			},

			handleConnectionStateChange: function(peerConnection, connectionState) {
				if(peerConnection.rtcUserDeleted) return;
				if(!peerConnection.rtcStateChangeCallback) return;
				var map = {
					'new': 0,
					'connecting': 1,
					'connected': 2,
					'disconnected': 3,
					'failed': 4,
					'closed': 5,
				};
				if(connectionState in map) {
					var stateChangeCallback = peerConnection.rtcStateChangeCallback;
					var userPointer = peerConnection.rtcUserPointer || 0;
					{{{ makeDynCall('vii', 'stateChangeCallback') }}} (map[connectionState], userPointer);
				}
			},

			handleGatheringStateChange: function(peerConnection, iceGatheringState) {
				if(peerConnection.rtcUserDeleted) return;
				if(!peerConnection.rtcGatheringStateChangeCallback) return;
				var map = {
					'new': 0,
					'gathering': 1,
					'complete': 2,
				};
				if(iceGatheringState in map) {
					var gatheringStateChangeCallback = peerConnection.rtcGatheringStateChangeCallback;
					var userPointer = peerConnection.rtcUserPointer || 0;
					{{{ makeDynCall('vii', 'gatheringStateChangeCallback') }}} (map[iceGatheringState], userPointer);
				}
			},

			handleSignalingStateChange: function(peerConnection, signalingState) {
				if(peerConnection.rtcUserDeleted) return;
				if(!peerConnection.rtcSignalingStateChangeCallback) return;
				var map = {
					'stable': 0,
					'have-local-offer': 1,
					'have-remote-offer': 2,
					'have-local-pranswer': 3,
					'have-remote-pranswer': 4,
				};
				if(signalingState in map) {
					var signalingStateChangeCallback = peerConnection.rtcSignalingStateChangeCallback;
					var userPointer = peerConnection.rtcUserPointer || 0;
					{{{ makeDynCall('vii', 'signalingStateChangeCallback') }}} (map[signalingState], userPointer);
				}
			},
		},


    getDataChannelOrdered: function(dc) {
      if(!dc) return;
      var dataChannel = WEBRTC.dataChannelsMap[dc];
      if(dataChannel) {
        return dataChannel.ordered;
      }
    },
    getDataChannelMaxPacketLifeTime: function(dc) {
      if(!dc) return;
      var dataChannel = WEBRTC.dataChannelsMap[dc];
      if(dataChannel) {
        return dataChannel.maxPacketLifeTime == null ? -1 : dataChannel.maxPacketLifeTime;
      }
    },
    getDataChannelMaxRetransmits: function(dc) {
      if(!dc) return;
      var dataChannel = WEBRTC.dataChannelsMap[dc];
      if(dataChannel) {
        return dataChannel.maxRetransmits == null ? -1 : dataChannel.maxRetransmits;
      }
    },

		rtcCreatePeerConnection: function(pUrls, pUsernames, pPasswords, nIceServers) {
			if(!window.RTCPeerConnection) return 0;
			var iceServers = [];
			for(var i = 0; i < nIceServers; ++i) {
				var heap = Module['HEAPU32'];
				var pUrl = heap[pUrls/heap.BYTES_PER_ELEMENT + i];
				var url = UTF8ToString(pUrl);
				var pUsername = heap[pUsernames/heap.BYTES_PER_ELEMENT + i];
				var username = UTF8ToString(pUsername);
				var pPassword = heap[pPasswords/heap.BYTES_PER_ELEMENT + i];
				var password = UTF8ToString(pPassword);
				if (username == "") {
					iceServers.push({
						urls: [url],
					});
				} else {
					iceServers.push({
						urls: [url],
						username: username,
						credential: password
					});
				}
			}
			var config = {
				iceServers: iceServers,
			};
			return WEBRTC.registerPeerConnection(new RTCPeerConnection(config));
		},

		rtcDeletePeerConnection: function(pc) {
			var peerConnection = WEBRTC.peerConnectionsMap[pc];
			if(peerConnection) {
				peerConnection.rtcUserDeleted = true;
				delete WEBRTC.peerConnectionsMap[pc];
			}
		},

		rtcGetLocalDescription: function(pc) {
			if(!pc) return 0;
			var peerConnection = WEBRTC.peerConnectionsMap[pc];
			var localDescription = peerConnection.localDescription;
			if(!localDescription) return 0;
			var sdp = WEBRTC.allocUTF8FromString(localDescription.sdp);
			// sdp should be freed later in c++.
			return sdp;
		},

		rtcGetLocalDescriptionType: function(pc) {
			if(!pc) return 0;
			var peerConnection = WEBRTC.peerConnectionsMap[pc];
			var localDescription = peerConnection.localDescription;
			if(!localDescription) return 0;
			var type = WEBRTC.allocUTF8FromString(localDescription.type);
			// type should be freed later in c++.
			return type;
		},

    rtcGetRemoteDescription: function(pc) {
			if(!pc) return 0;
			var peerConnection = WEBRTC.peerConnectionsMap[pc];
			var remoteDescription = peerConnection.remoteDescription;
			if(!remoteDescription) return 0;
			var sdp = WEBRTC.allocUTF8FromString(remoteDescription.sdp);
			// sdp should be freed later in c++.
			return sdp;
		},

		rtcGetRemoteDescriptionType: function(pc) {
			if(!pc) return 0;
			var peerConnection = WEBRTC.peerConnectionsMap[pc];
			var remoteDescription = peerConnection.remoteDescription;
			if(!remoteDescription) return 0;
			var type = WEBRTC.allocUTF8FromString(remoteDescription.type);
			// type should be freed later in c++.
			return type;
		},

		rtcCreateDataChannel: function(pc, pLabel, unordered, maxRetransmits, maxPacketLifeTime) {
			if(!pc) return 0;
			var label = UTF8ToString(pLabel);
			var peerConnection = WEBRTC.peerConnectionsMap[pc];
			var datachannelInit = {
				ordered: !unordered,
			};

			// Browsers throw an exception when both are present (even if set to null)
			if (maxRetransmits >= 0) datachannelInit.maxRetransmits = maxRetransmits;
			else if (maxPacketLifeTime >= 0) datachannelInit.maxPacketLifeTime = maxPacketLifeTime;

			var channel = peerConnection.createDataChannel(label, datachannelInit);
			return WEBRTC.registerDataChannel(channel);
		},

 		rtcDeleteDataChannel: function(dc) {
			var dataChannel = WEBRTC.dataChannelsMap[dc];
			if(dataChannel) {
				dataChannel.rtcUserDeleted = true;
				delete WEBRTC.dataChannelsMap[dc];
			}
		},

		rtcSetDataChannelCallback: function(pc, dataChannelCallback) {
			if(!pc) return;
			var peerConnection = WEBRTC.peerConnectionsMap[pc];
			peerConnection.ondatachannel = function(evt) {
				if(peerConnection.rtcUserDeleted) return;
				var dc = WEBRTC.registerDataChannel(evt.channel);
				var userPointer = peerConnection.rtcUserPointer || 0;
				{{{ makeDynCall('vii', 'dataChannelCallback') }}} (dc, userPointer);
			};
		},

		rtcSetLocalDescriptionCallback: function(pc, descriptionCallback) {
			if(!pc) return;
			var peerConnection = WEBRTC.peerConnectionsMap[pc];
			peerConnection.rtcDescriptionCallback = descriptionCallback;
		},

		rtcSetLocalCandidateCallback: function(pc, candidateCallback) {
			if(!pc) return;
			var peerConnection = WEBRTC.peerConnectionsMap[pc];
			peerConnection.rtcCandidateCallback = candidateCallback;
		},

		rtcSetStateChangeCallback: function(pc, stateChangeCallback) {
			if(!pc) return;
			var peerConnection = WEBRTC.peerConnectionsMap[pc];
			peerConnection.rtcStateChangeCallback = stateChangeCallback;
		},

		rtcSetGatheringStateChangeCallback: function(pc, gatheringStateChangeCallback) {
			if(!pc) return;
			var peerConnection = WEBRTC.peerConnectionsMap[pc];
			peerConnection.rtcGatheringStateChangeCallback = gatheringStateChangeCallback;
		},

		rtcSetSignalingStateChangeCallback: function(pc, signalingStateChangeCallback) {
			if(!pc) return;
			var peerConnection = WEBRTC.peerConnectionsMap[pc];
			peerConnection.rtcSignalingStateChangeCallback = signalingStateChangeCallback;
		},

		rtcSetRemoteDescription: function(pc, pSdp, pType) {
			var description = new RTCSessionDescription({
				sdp: UTF8ToString(pSdp),
				type: UTF8ToString(pType),
			});
			var peerConnection = WEBRTC.peerConnectionsMap[pc];
			peerConnection.setRemoteDescription(description)
				.then(function() {
					if(peerConnection.rtcUserDeleted) return;
					if(description.type == 'offer') {
						peerConnection.createAnswer()
							.then(function(answer) {
								return WEBRTC.handleDescription(peerConnection, answer);
							})
							.catch(function(err) {
								console.error(err);
							});
					}
				})
				.catch(function(err) {
					console.error(err);
				});
		},

		rtcAddRemoteCandidate: function(pc, pCandidate, pSdpMid) {
			var iceCandidate = new RTCIceCandidate({
				candidate: UTF8ToString(pCandidate),
				sdpMid: UTF8ToString(pSdpMid),
			});
			var peerConnection = WEBRTC.peerConnectionsMap[pc];
			peerConnection.addIceCandidate(iceCandidate)
				.catch(function(err) {
					console.error(err);
				});
		},

		rtcGetDataChannelLabel: function(dc, pBuffer, size) {
			var label = WEBRTC.dataChannelsMap[dc].label;
			stringToUTF8(label, pBuffer, size);
			return lengthBytesUTF8(label);
		},

		rtcSetOpenCallback: function(dc, openCallback) {
			var dataChannel = WEBRTC.dataChannelsMap[dc];
			var cb = function() {
				if(dataChannel.rtcUserDeleted) return;
				var userPointer = dataChannel.rtcUserPointer || 0;
				{{{ makeDynCall('vi', 'openCallback') }}} (userPointer);
			};
			dataChannel.onopen = cb;
			if(dataChannel.readyState == 'open') setTimeout(cb, 0);
		},

		rtcSetErrorCallback: function(dc, errorCallback) {
			var dataChannel = WEBRTC.dataChannelsMap[dc];
			var cb = function(evt) {
				if(dataChannel.rtcUserDeleted) return;
				var userPointer = dataChannel.rtcUserPointer || 0;
				var pError = evt.message ? WEBRTC.allocUTF8FromString(evt.message) : 0;
				{{{ makeDynCall('vii', 'errorCallback') }}} (pError, userPointer);
				_free(pError);
			};
			dataChannel.onerror = cb;
		},

		rtcSetMessageCallback: function(dc, messageCallback) {
			var dataChannel = WEBRTC.dataChannelsMap[dc];
			dataChannel.onmessage = function(evt) {
				if(dataChannel.rtcUserDeleted) return;
				var userPointer = dataChannel.rtcUserPointer || 0;
				if(typeof evt.data == 'string') {
					var pStr = WEBRTC.allocUTF8FromString(evt.data);
					{{{ makeDynCall('viii', 'messageCallback') }}} (pStr, -1, userPointer);
					_free(pStr);
				} else {
					var byteArray = new Uint8Array(evt.data);
					var size = byteArray.length;
					var pBuffer = _malloc(size);
					var heapBytes = new Uint8Array(Module['HEAPU8'].buffer, pBuffer, size);
					heapBytes.set(byteArray);
					{{{ makeDynCall('viii', 'messageCallback') }}} (pBuffer, size, userPointer);
					_free(pBuffer);
				}
			};
			dataChannel.onclose = function() {
				if(dataChannel.rtcUserDeleted) return;
				var userPointer = dataChannel.rtcUserPointer || 0;
				{{{ makeDynCall('viii', 'messageCallback') }}} (0, 0, userPointer);
			};
		},

		rtcSetBufferedAmountLowCallback: function(dc, bufferedAmountLowCallback) {
			var dataChannel = WEBRTC.dataChannelsMap[dc];
			var cb = function(evt) {
				if(dataChannel.rtcUserDeleted) return;
				var userPointer = dataChannel.rtcUserPointer || 0;
				{{{ makeDynCall('vi', 'bufferedAmountLowCallback') }}} (userPointer);
			};
			dataChannel.onbufferedamountlow = cb;
		},

		rtcGetBufferedAmount: function(dc) {
			var dataChannel = WEBRTC.dataChannelsMap[dc];
			return dataChannel.bufferedAmount;
		},

		rtcSetBufferedAmountLowThreshold: function(dc, threshold) {
			var dataChannel = WEBRTC.dataChannelsMap[dc];
			dataChannel.bufferedAmountLowThreshold = threshold;
		},

		rtcSendMessage: function(dc, pBuffer, size) {
			var dataChannel = WEBRTC.dataChannelsMap[dc];
			if(dataChannel.readyState != 'open') return -1;
			if(size >= 0) {
				var heapBytes = new Uint8Array(Module['HEAPU8'].buffer, pBuffer, size);
				dataChannel.send(heapBytes);
				return size;
			} else {
				var str = UTF8ToString(pBuffer);
				dataChannel.send(str);
				return lengthBytesUTF8(str);
			}
		},

		rtcSetUserPointer: function(i, ptr) {
			if(WEBRTC.peerConnectionsMap[i]) WEBRTC.peerConnectionsMap[i].rtcUserPointer = ptr;
			if(WEBRTC.dataChannelsMap[i]) WEBRTC.dataChannelsMap[i].rtcUserPointer = ptr;
		},
	};

	autoAddDeps(WebRTC, '$WEBRTC');
	mergeInto(LibraryManager.library, WebRTC);
})();
