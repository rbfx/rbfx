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

#ifndef RTC_PEERCONNECTION_H
#define RTC_PEERCONNECTION_H

#include "candidate.hpp"
#include "common.hpp"
#include "configuration.hpp"
#include "datachannel.hpp"
#include "description.hpp"
#include "reliability.hpp"

#include <functional>
#include <optional>
#include <variant>

namespace rtc {

struct DataChannelInit {
	Reliability reliability = {};
};

class PeerConnection final {
public:
	enum class State : int {
		New = 0,
		Connecting = 1,
		Connected = 2,
		Disconnected = 3,
		Failed = 4,
		Closed = 5
	};

	enum class GatheringState : int { New = 0, InProgress = 1, Complete = 2 };

	enum class SignalingState : int {
		Stable = 0,
		HaveLocalOffer = 1,
		HaveRemoteOffer = 2,
		HaveLocalPranswer = 3,
		HaveRemotePranswer = 4,
	};

	PeerConnection();
	PeerConnection(const Configuration &config);
	~PeerConnection();

	State state() const;
	GatheringState gatheringState() const;
	SignalingState signalingState() const;
	optional<Description> localDescription() const;
	optional<Description> remoteDescription() const;

	shared_ptr<DataChannel> createDataChannel(const string &label, DataChannelInit init = {});

	void setRemoteDescription(const Description &description);
	void addRemoteCandidate(const Candidate &candidate);

	void onDataChannel(std::function<void(shared_ptr<DataChannel>)> callback);
	void onLocalDescription(std::function<void(const Description &description)> callback);
	void onLocalCandidate(std::function<void(const Candidate &candidate)> callback);
	void onStateChange(std::function<void(State state)> callback);
	void onGatheringStateChange(std::function<void(GatheringState state)> callback);
	void onSignalingStateChange(std::function<void(SignalingState state)> callback);

protected:
	void triggerDataChannel(shared_ptr<DataChannel> dataChannel);
	void triggerLocalDescription(const Description &description);
	void triggerLocalCandidate(const Candidate &candidate);
	void triggerStateChange(State state);
	void triggerGatheringStateChange(GatheringState state);
	void triggerSignalingStateChange(SignalingState state);

	std::function<void(shared_ptr<DataChannel>)> mDataChannelCallback;
	std::function<void(const Description &description)> mLocalDescriptionCallback;
	std::function<void(const Candidate &candidate)> mLocalCandidateCallback;
	std::function<void(State candidate)> mStateChangeCallback;
	std::function<void(GatheringState candidate)> mGatheringStateChangeCallback;
	std::function<void(SignalingState candidate)> mSignalingStateChangeCallback;

private:
	int mId;
	State mState = State::New;
	GatheringState mGatheringState = GatheringState::New;
	SignalingState mSignalingState = SignalingState::Stable;

	static void DataChannelCallback(int dc, void *ptr);
	static void DescriptionCallback(const char *sdp, const char *type, void *ptr);
	static void CandidateCallback(const char *candidate, const char *mid, void *ptr);
	static void StateChangeCallback(int state, void *ptr);
	static void GatheringStateChangeCallback(int state, void *ptr);
	static void SignalingStateChangeCallback(int state, void *ptr);
};

} // namespace rtc

#endif // RTC_PEERCONNECTION_H
