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

#include "peerconnection.hpp"

#include <emscripten/emscripten.h>

#include <exception>
#include <iostream>
#include <stdexcept>

extern "C" {
extern int rtcCreatePeerConnection(const char **pUrls, const char **pUsernames,
                                   const char **pPasswords, int nIceServers);
extern void rtcDeletePeerConnection(int pc);
extern char *rtcGetLocalDescription(int pc);
extern char *rtcGetLocalDescriptionType(int pc);
extern char *rtcGetRemoteDescription(int pc);
extern char *rtcGetRemoteDescriptionType(int pc);
extern int rtcCreateDataChannel(int pc, const char *label, bool unordered, int maxRetransmits,
                                int maxPacketLifeTime);
extern void rtcSetDataChannelCallback(int pc, void (*dataChannelCallback)(int, void *));
extern void rtcSetLocalDescriptionCallback(int pc,
                                           void (*descriptionCallback)(const char *, const char *,
                                                                       void *));
extern void rtcSetLocalCandidateCallback(int pc, void (*candidateCallback)(const char *,
                                                                           const char *, void *));
extern void rtcSetStateChangeCallback(int pc, void (*stateChangeCallback)(int, void *));
extern void rtcSetGatheringStateChangeCallback(int pc,
                                               void (*gatheringStateChangeCallback)(int, void *));
extern void rtcSetSignalingStateChangeCallback(int pc,
                                               void (*signalingStateChangeCallback)(int, void *));
extern void rtcSetRemoteDescription(int pc, const char *sdp, const char *type);
extern void rtcAddRemoteCandidate(int pc, const char *candidate, const char *mid);
extern void rtcSetUserPointer(int i, void *ptr);
}

namespace rtc {

using std::function;
using std::vector;

void PeerConnection::DataChannelCallback(int dc, void *ptr) {
	PeerConnection *p = static_cast<PeerConnection *>(ptr);
	if (p)
		p->triggerDataChannel(std::make_shared<DataChannel>(dc));
}

void PeerConnection::DescriptionCallback(const char *sdp, const char *type, void *ptr) {
	PeerConnection *p = static_cast<PeerConnection *>(ptr);
	if (p)
		p->triggerLocalDescription(Description(sdp, type));
}

void PeerConnection::CandidateCallback(const char *candidate, const char *mid, void *ptr) {
	PeerConnection *p = static_cast<PeerConnection *>(ptr);
	if (p)
		p->triggerLocalCandidate(Candidate(candidate, mid));
}

void PeerConnection::StateChangeCallback(int state, void *ptr) {
	PeerConnection *p = static_cast<PeerConnection *>(ptr);
	if (p)
		p->triggerStateChange(static_cast<State>(state));
}

void PeerConnection::GatheringStateChangeCallback(int state, void *ptr) {
	PeerConnection *p = static_cast<PeerConnection *>(ptr);
	if (p)
		p->triggerGatheringStateChange(static_cast<GatheringState>(state));
}

void PeerConnection::SignalingStateChangeCallback(int state, void *ptr) {
	PeerConnection *p = static_cast<PeerConnection *>(ptr);
	if (p)
		p->triggerSignalingStateChange(static_cast<SignalingState>(state));
}

PeerConnection::PeerConnection(const Configuration &config) {
	vector<string> urls;
	urls.reserve(config.iceServers.size());
	for (const IceServer &iceServer : config.iceServers) {
		string url;
		if (iceServer.type == IceServer::Type::Dummy) {
			url = iceServer.hostname;
		} else {
			string scheme =
			    iceServer.type == IceServer::Type::Turn
			        ? (iceServer.relayType == IceServer::RelayType::TurnTls ? "turns" : "turn")
			        : "stun";

			url += scheme + ":" + iceServer.hostname;

			if (iceServer.port != 0)
				url += string(":") + std::to_string(iceServer.port);

			if (iceServer.type == IceServer::Type::Turn &&
			    iceServer.relayType != IceServer::RelayType::TurnUdp)
				url += "?transport=tcp";
		}
		urls.push_back(url);
	}

	vector<const char *> url_ptrs;
	vector<const char *> username_ptrs;
	vector<const char *> password_ptrs;
	url_ptrs.reserve(config.iceServers.size());
	username_ptrs.reserve(config.iceServers.size());
	password_ptrs.reserve(config.iceServers.size());
	for (const string &s : urls)
		url_ptrs.push_back(s.c_str());
	for (const IceServer &iceServer : config.iceServers) {
		username_ptrs.push_back(iceServer.username.c_str());
		password_ptrs.push_back(iceServer.password.c_str());
	}
	mId = rtcCreatePeerConnection(url_ptrs.data(), username_ptrs.data(), password_ptrs.data(),
	                              config.iceServers.size());
	if (!mId)
		throw std::runtime_error("WebRTC not supported");

	rtcSetUserPointer(mId, this);
	rtcSetDataChannelCallback(mId, DataChannelCallback);
	rtcSetLocalDescriptionCallback(mId, DescriptionCallback);
	rtcSetLocalCandidateCallback(mId, CandidateCallback);
	rtcSetStateChangeCallback(mId, StateChangeCallback);
	rtcSetGatheringStateChangeCallback(mId, GatheringStateChangeCallback);
	rtcSetSignalingStateChangeCallback(mId, SignalingStateChangeCallback);
}

PeerConnection::~PeerConnection() { rtcDeletePeerConnection(mId); }

PeerConnection::State PeerConnection::state() const { return mState; }

PeerConnection::GatheringState PeerConnection::gatheringState() const { return mGatheringState; }

PeerConnection::SignalingState PeerConnection::signalingState() const { return mSignalingState; }

optional<Description> PeerConnection::localDescription() const {
	char *sdp = rtcGetLocalDescription(mId);
	char *type = rtcGetLocalDescriptionType(mId);
	if (!sdp || !type) {
		free(sdp);
		free(type);
		return std::nullopt;
	}
	Description description(sdp, type);
	free(sdp);
	free(type);
	return description;
}

optional<Description> PeerConnection::remoteDescription() const {
	char *sdp = rtcGetRemoteDescription(mId);
	char *type = rtcGetRemoteDescriptionType(mId);
	if (!sdp || !type) {
		free(sdp);
		free(type);
		return std::nullopt;
	}
	Description description(sdp, type);
	free(sdp);
	free(type);
	return description;
}

shared_ptr<DataChannel> PeerConnection::createDataChannel(const string &label,
                                                          DataChannelInit init) {
	int maxRetransmits = init.reliability.type == Reliability::Type::Rexmit
	                         ? std::get<int>(init.reliability.rexmit)
	                         : -1;
	int maxPacketLifeTime =
	    init.reliability.type == Reliability::Type::Timed
	        ? int(std::get<std::chrono::milliseconds>(init.reliability.rexmit).count())
	        : -1;
	return std::make_shared<DataChannel>(rtcCreateDataChannel(
	    mId, label.c_str(), init.reliability.unordered, maxRetransmits, maxPacketLifeTime));
}

void PeerConnection::setRemoteDescription(const Description &description) {
	rtcSetRemoteDescription(mId, string(description).c_str(), description.typeString().c_str());
}

void PeerConnection::addRemoteCandidate(const Candidate &candidate) {
	rtcAddRemoteCandidate(mId, candidate.candidate().c_str(), candidate.mid().c_str());
}

void PeerConnection::onDataChannel(function<void(shared_ptr<DataChannel>)> callback) {
	mDataChannelCallback = callback;
}

void PeerConnection::onLocalDescription(function<void(const Description &)> callback) {
	mLocalDescriptionCallback = callback;
}

void PeerConnection::onLocalCandidate(function<void(const Candidate &)> callback) {
	mLocalCandidateCallback = callback;
}

void PeerConnection::onStateChange(function<void(State state)> callback) {
	mStateChangeCallback = callback;
}

void PeerConnection::onGatheringStateChange(function<void(GatheringState state)> callback) {
	mGatheringStateChangeCallback = callback;
}

void PeerConnection::onSignalingStateChange(function<void(SignalingState state)> callback) {
	mSignalingStateChangeCallback = callback;
}

void PeerConnection::triggerDataChannel(shared_ptr<DataChannel> dataChannel) {
	if (mDataChannelCallback)
		mDataChannelCallback(dataChannel);
}

void PeerConnection::triggerLocalDescription(const Description &description) {
	if (mLocalDescriptionCallback)
		mLocalDescriptionCallback(description);
}

void PeerConnection::triggerLocalCandidate(const Candidate &candidate) {
	if (mLocalCandidateCallback)
		mLocalCandidateCallback(candidate);
}

void PeerConnection::triggerStateChange(State state) {
	mState = state;
	if (mStateChangeCallback)
		mStateChangeCallback(state);
}

void PeerConnection::triggerGatheringStateChange(GatheringState state) {
	mGatheringState = state;
	if (mGatheringStateChangeCallback)
		mGatheringStateChangeCallback(state);
}

void PeerConnection::triggerSignalingStateChange(SignalingState state) {
	mSignalingState = state;
	if (mSignalingStateChangeCallback)
		mSignalingStateChangeCallback(state);
}
} // namespace rtc
