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

#include <EASTL/sort.h>

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Graphics/Animation.h"
#include "../IO/Deserializer.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../IO/Serializer.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "../Resource/JSONFile.h"

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

inline bool CompareTriggers(const AnimationTriggerPoint& lhs, const AnimationTriggerPoint& rhs)
{
    return lhs.time_ < rhs.time_;
}

void ReadTransform(Deserializer& source, Transform& transform, AnimationChannelFlags channelMask)
{
    if (channelMask & CHANNEL_POSITION)
        transform.position_ = source.ReadVector3();
    if (channelMask & CHANNEL_ROTATION)
        transform.rotation_ = source.ReadQuaternion();
    if (channelMask & CHANNEL_SCALE)
        transform.scale_ = source.ReadVector3();
}

void WriteTransform(Serializer& dest, const Transform& transform, AnimationChannelFlags channelMask)
{
    if (channelMask & CHANNEL_POSITION)
        dest.WriteVector3(transform.position_);
    if (channelMask & CHANNEL_ROTATION)
        dest.WriteQuaternion(transform.rotation_);
    if (channelMask & CHANNEL_SCALE)
        dest.WriteVector3(transform.scale_);
}

Variant InterpolateSpline(VariantType type,
    const Variant& v1, const Variant& v2, const Variant& t1, const Variant& t2, float t)
{
    const float tt = t * t;
    const float ttt = t * tt;

    const float h1 = 2.0f * ttt - 3.0f * tt + 1.0f;
    const float h2 = -2.0f * ttt + 3.0f * tt;
    const float h3 = ttt - 2.0f * tt + t;
    const float h4 = ttt - tt;

    switch (type)
    {
    case VAR_FLOAT:
        return v1.GetFloat() * h1 + v2.GetFloat() * h2 + t1.GetFloat() * h3 + t2.GetFloat() * h4;

    case VAR_VECTOR2:
        return v1.GetVector2() * h1 + v2.GetVector2() * h2 + t1.GetVector2() * h3 + t2.GetVector2() * h4;

    case VAR_VECTOR3:
        return v1.GetVector3() * h1 + v2.GetVector3() * h2 + t1.GetVector3() * h3 + t2.GetVector3() * h4;

    case VAR_VECTOR4:
        return v1.GetVector4() * h1 + v2.GetVector4() * h2 + t1.GetVector4() * h3 + t2.GetVector4() * h4;

    case VAR_QUATERNION:
        return v1.GetQuaternion() * h1 + v2.GetQuaternion() * h2 + t1.GetQuaternion() * h3 + t2.GetQuaternion() * h4;

    case VAR_COLOR:
        return v1.GetColor() * h1 + v2.GetColor() * h2 + t1.GetColor() * h3 + t2.GetColor() * h4;

    case VAR_DOUBLE:
        return v1.GetDouble() * h1 + v2.GetDouble() * h2 + t1.GetDouble() * h3 + t2.GetDouble() * h4;

    default:
        return v1;
    }
}

Variant SubstractAndMultiply(VariantType type, const Variant& v1, const Variant& v2, float t)
{
    switch (type)
    {
    case VAR_FLOAT:
        return (v1.GetFloat() - v2.GetFloat()) * t;

    case VAR_VECTOR2:
        return (v1.GetVector2() - v2.GetVector2()) * t;

    case VAR_VECTOR3:
        return (v1.GetVector3() - v2.GetVector3()) * t;

    case VAR_VECTOR4:
        return (v1.GetVector4() - v2.GetVector4()) * t;

    case VAR_QUATERNION:
        return (v1.GetQuaternion() - v2.GetQuaternion()) * t;

    case VAR_COLOR:
        return (v1.GetColor() - v2.GetColor()) * t;

    case VAR_DOUBLE:
        return (v1.GetDouble() - v2.GetDouble()) * t;

    default:
        return Variant(type);
    }
}

}

void AnimationTrack::Sample(float time, float duration, bool isLooped, unsigned& frameIndex, Transform& value) const
{
    float blendFactor{};
    unsigned nextFrameIndex{};
    GetKeyFrames(time, duration, isLooped, frameIndex, nextFrameIndex, blendFactor);

    const AnimationKeyFrame& keyFrame = keyFrames_[frameIndex];
    const AnimationKeyFrame& nextKeyFrame = keyFrames_[nextFrameIndex];

    if (blendFactor >= M_EPSILON)
    {
        if (channelMask_ & CHANNEL_POSITION)
            value.position_ = keyFrame.position_.Lerp(nextKeyFrame.position_, blendFactor);
        if (channelMask_ & CHANNEL_ROTATION)
            value.rotation_ = keyFrame.rotation_.Slerp(nextKeyFrame.rotation_, blendFactor);
        if (channelMask_ & CHANNEL_SCALE)
            value.scale_ = keyFrame.scale_.Lerp(nextKeyFrame.scale_, blendFactor);
    }
    else
    {
        if (channelMask_ & CHANNEL_POSITION)
            value.position_ = keyFrame.position_;
        if (channelMask_ & CHANNEL_ROTATION)
            value.rotation_ = keyFrame.rotation_;
        if (channelMask_ & CHANNEL_SCALE)
            value.scale_ = keyFrame.scale_;
    }
}

void VariantAnimationTrack::Commit()
{
    type_ = GetType();

    switch (type_)
    {
    case VAR_FLOAT:
    case VAR_VECTOR2:
    case VAR_VECTOR3:
    case VAR_VECTOR4:
    case VAR_QUATERNION:
    case VAR_COLOR:
    case VAR_DOUBLE:
        // Floating point compounds may have any interpolation type.
        // Calculate tangents if spline interpolation is used.
        if (interpolation_ == KeyFrameInterpolation::Spline)
        {
            const unsigned numKeyFrames = keyFrames_.size();
            splineTangents_.resize(numKeyFrames);

            for (unsigned i = 1; i + 1 < numKeyFrames; ++i)
            {
                splineTangents_[i] = SubstractAndMultiply(
                    type_, keyFrames_[i + 1].value_, keyFrames_[i - 1].value_, splineTension_);
            }

            // If spline is not closed, make end point's tangent zero
            if (numKeyFrames <= 2 || keyFrames_[0].value_ != keyFrames_[numKeyFrames - 1].value_)
            {
                splineTangents_[0] = Variant(type_);
                if (numKeyFrames >= 2)
                    splineTangents_[numKeyFrames - 1] = Variant(type_);
            }
            else
            {
                const Variant tangent = SubstractAndMultiply(
                    type_, keyFrames_[1].value_, keyFrames_[numKeyFrames - 2].value_, splineTension_);
                splineTangents_[0] = tangent;
                splineTangents_[numKeyFrames - 1] = tangent;
            }
        }
        break;

    case VAR_INT:
    case VAR_INT64:
    case VAR_INTRECT:
    case VAR_INTVECTOR2:
    case VAR_INTVECTOR3:
        // Integer compounds cannot have spline interpolation, fallback to linear.
        if (interpolation_ == KeyFrameInterpolation::Spline)
            interpolation_ = KeyFrameInterpolation::Linear;
        break;

    default:
        // Other types don't support interpolation at all, fallback to none.
        interpolation_ = KeyFrameInterpolation::None;
        break;
    }
}

Variant VariantAnimationTrack::Sample(float time, float duration, bool isLooped, unsigned& frameIndex) const
{
    float blendFactor{};
    unsigned nextFrameIndex{};
    GetKeyFrames(time, duration, isLooped, frameIndex, nextFrameIndex, blendFactor);

    const VariantAnimationKeyFrame& keyFrame = keyFrames_[frameIndex];
    const VariantAnimationKeyFrame& nextKeyFrame = keyFrames_[nextFrameIndex];

    if (blendFactor >= M_EPSILON)
    {
        if (interpolation_ == KeyFrameInterpolation::Spline && splineTangents_.size() == keyFrames_.size())
        {
            return InterpolateSpline(type_, keyFrame.value_, nextKeyFrame.value_,
                splineTangents_[frameIndex], splineTangents_[nextFrameIndex], blendFactor);
        }
        else if (interpolation_ == KeyFrameInterpolation::Linear)
            return keyFrame.value_.Lerp(nextKeyFrame.value_, blendFactor);
    }

    return keyFrame.value_;
}

VariantType VariantAnimationTrack::GetType() const
{
    return keyFrames_.empty() ? VAR_NONE : keyFrames_[0].value_.GetType();
}

Animation::Animation(Context* context) :
    ResourceWithMetadata(context),
    length_(0.f)
{
}

Animation::~Animation() = default;

void Animation::RegisterObject(Context* context)
{
    context->RegisterFactory<Animation>();
}

bool Animation::BeginLoad(Deserializer& source)
{
    unsigned memoryUse = sizeof(Animation);

    // Check ID
    const ea::string fileID = source.ReadFileID();
    if (fileID != "UANI" && fileID != "UAN2")
    {
        URHO3D_LOGERROR(source.GetName() + " is not a valid animation file");
        return false;
    }

    // Read version
    const unsigned version = fileID == "UAN2" ? source.ReadUInt() : legacyVersion;

    // Read name and length
    animationName_ = source.ReadString();
    animationNameHash_ = animationName_;
    length_ = source.ReadFloat();
    tracks_.clear();

    const unsigned tracks = source.ReadUInt();
    memoryUse += tracks * sizeof(AnimationTrack);

    // Read tracks
    for (unsigned i = 0; i < tracks; ++i)
    {
        AnimationTrack* newTrack = CreateTrack(source.ReadString());
        newTrack->channelMask_ = AnimationChannelFlags(source.ReadUByte());

        if (version >= variantTrackVersion)
            ReadTransform(source, newTrack->baseValue_, newTrack->channelMask_);

        const unsigned keyFrames = source.ReadUInt();
        newTrack->keyFrames_.resize(keyFrames);
        memoryUse += keyFrames * sizeof(AnimationKeyFrame);

        // Read keyframes of the track
        for (unsigned j = 0; j < keyFrames; ++j)
        {
            AnimationKeyFrame& newKeyFrame = newTrack->keyFrames_[j];
            newKeyFrame.time_ = source.ReadFloat();
            ReadTransform(source, newKeyFrame, newTrack->channelMask_);
        }
    }

    // Read variant tracks
    if (version >= variantTrackVersion)
    {
        const unsigned variantTracks = source.ReadUInt();
        memoryUse += variantTracks * sizeof(VariantAnimationTrack);

        for (unsigned i = 0; i < variantTracks; ++i)
        {
            VariantAnimationTrack* newTrack = CreateVariantTrack(source.ReadString());
            const auto trackType = static_cast<VariantType>(source.ReadUByte());

            newTrack->interpolation_ = static_cast<KeyFrameInterpolation>(source.ReadUByte());
            newTrack->splineTension_ = source.ReadFloat();
            newTrack->baseValue_ = source.ReadVariant(trackType);

            const unsigned keyFrames = source.ReadUInt();
            newTrack->keyFrames_.resize(keyFrames);
            memoryUse += keyFrames * sizeof(VariantAnimationKeyFrame);

            // Read keyframes of the track
            for (unsigned j = 0; j < keyFrames; ++j)
            {
                VariantAnimationKeyFrame& newKeyFrame = newTrack->keyFrames_[j];
                newKeyFrame.time_ = source.ReadFloat();
                newKeyFrame.value_ = source.ReadVariant(trackType);
            }

            newTrack->Commit();
        }
    }

    // Optionally read triggers from an XML file
    auto* cache = GetSubsystem<ResourceCache>();
    ea::string xmlName = ReplaceExtension(GetName(), ".xml");

    SharedPtr<XMLFile> file(cache->GetTempResource<XMLFile>(xmlName, false));
    if (file)
    {
        XMLElement rootElem = file->GetRoot();
        for (XMLElement triggerElem = rootElem.GetChild("trigger"); triggerElem; triggerElem = triggerElem.GetNext("trigger"))
        {
            if (triggerElem.HasAttribute("normalizedtime"))
                AddTrigger(triggerElem.GetFloat("normalizedtime"), true, triggerElem.GetVariant());
            else if (triggerElem.HasAttribute("time"))
                AddTrigger(triggerElem.GetFloat("time"), false, triggerElem.GetVariant());
        }

        LoadMetadataFromXML(rootElem);

        memoryUse += triggers_.size() * sizeof(AnimationTriggerPoint);
        SetMemoryUse(memoryUse);
        return true;
    }

    SetMemoryUse(memoryUse);
    return true;
}

bool Animation::Save(Serializer& dest) const
{
    // Write ID, name and length
    dest.WriteFileID("UAN2");
    dest.WriteUInt(currentVersion);
    dest.WriteString(animationName_);
    dest.WriteFloat(length_);

    // Write tracks
    dest.WriteUInt(tracks_.size());
    for (const auto& item : tracks_)
    {
        const AnimationTrack& track = item.second;
        dest.WriteString(track.name_);
        dest.WriteUByte(track.channelMask_);
        WriteTransform(dest, track.baseValue_, track.channelMask_);
        dest.WriteUInt(track.keyFrames_.size());

        // Write keyframes of the track
        for (unsigned j = 0; j < track.keyFrames_.size(); ++j)
        {
            const AnimationKeyFrame& keyFrame = track.keyFrames_[j];
            dest.WriteFloat(keyFrame.time_);
            WriteTransform(dest, keyFrame, track.channelMask_);
        }
    }

    // Write variant tracks
    dest.WriteUInt(variantTracks_.size());
    for (const auto& item : variantTracks_)
    {
        const VariantAnimationTrack& track = item.second;
        const VariantType trackType = track.GetType();
        const Variant defaultValue(trackType);

        dest.WriteString(track.name_);
        dest.WriteUByte(static_cast<unsigned char>(trackType));
        dest.WriteUByte(static_cast<unsigned char>(track.interpolation_));
        dest.WriteFloat(track.splineTension_);
        dest.WriteVariantData(track.baseValue_.GetType() == trackType ? track.baseValue_ : defaultValue);
        dest.WriteUInt(track.keyFrames_.size());

        // Write keyframes of the track
        for (unsigned j = 0; j < track.keyFrames_.size(); ++j)
        {
            const VariantAnimationKeyFrame& keyFrame = track.keyFrames_[j];
            dest.WriteFloat(keyFrame.time_);
            dest.WriteVariantData(keyFrame.value_.GetType() == trackType ? keyFrame.value_ : defaultValue);
        }
    }

    // If triggers have been defined, write an XML file for them
    if (!triggers_.empty() || HasMetadata())
    {
        auto* destFile = dynamic_cast<File*>(&dest);
        if (destFile)
        {
            ea::string xmlName = ReplaceExtension(destFile->GetName(), ".xml");

            SharedPtr<XMLFile> xml(context_->CreateObject<XMLFile>());
            XMLElement rootElem = xml->CreateRoot("animation");

            for (unsigned i = 0; i < triggers_.size(); ++i)
            {
                XMLElement triggerElem = rootElem.CreateChild("trigger");
                triggerElem.SetFloat("time", triggers_[i].time_);
                triggerElem.SetVariant(triggers_[i].data_);
            }

            SaveMetadataToXML(rootElem);

            File xmlFile(context_, xmlName, FILE_WRITE);
            xml->Save(xmlFile);
        }
        else
            URHO3D_LOGWARNING("Can not save animation trigger data when not saving into a file");
    }

    return true;
}

void Animation::SetAnimationName(const ea::string& name)
{
    animationName_ = name;
    animationNameHash_ = StringHash(name);
}

void Animation::SetLength(float length)
{
    length_ = Max(length, 0.0f);
}

AnimationTrack* Animation::CreateTrack(const ea::string& name)
{
    /// \todo When tracks / keyframes are created dynamically, memory use is not updated
    static const AnimationTrack defaultTrack;
    const auto [iter, isNew] = tracks_.emplace(name, defaultTrack);

    if (isNew)
    {
        iter->second.name_ = name;
        iter->second.nameHash_ = iter->first;
    }

    return &iter->second;
}

VariantAnimationTrack* Animation::CreateVariantTrack(const ea::string& name)
{
    /// \todo When tracks / keyframes are created dynamically, memory use is not updated
    static const VariantAnimationTrack defaultTrack;
    const auto [iter, isNew] = variantTracks_.emplace(name, defaultTrack);

    if (isNew)
    {
        iter->second.name_ = name;
        iter->second.nameHash_ = iter->first;
    }

    return &iter->second;
}

bool Animation::RemoveTrack(const ea::string& name)
{
    auto i = tracks_.find(StringHash(name));
    if (i != tracks_.end())
    {
        tracks_.erase(i);
        return true;
    }
    else
        return false;
}

void Animation::RemoveAllTracks()
{
    tracks_.clear();
}

void Animation::SetTrigger(unsigned index, const AnimationTriggerPoint& trigger)
{
    if (index == triggers_.size())
        AddTrigger(trigger);
    else if (index < triggers_.size())
    {
        triggers_[index] = trigger;
        ea::quick_sort(triggers_.begin(), triggers_.end(), CompareTriggers);
    }
}

void Animation::AddTrigger(const AnimationTriggerPoint& trigger)
{
    triggers_.push_back(trigger);
    ea::quick_sort(triggers_.begin(), triggers_.end(), CompareTriggers);
}

void Animation::AddTrigger(float time, bool timeIsNormalized, const Variant& data)
{
    AnimationTriggerPoint newTrigger;
    newTrigger.time_ = timeIsNormalized ? time * length_ : time;
    newTrigger.data_ = data;
    triggers_.push_back(newTrigger);

    ea::quick_sort(triggers_.begin(), triggers_.end(), CompareTriggers);
}

void Animation::RemoveTrigger(unsigned index)
{
    if (index < triggers_.size())
        triggers_.erase_at(index);
}

void Animation::RemoveAllTriggers()
{
    triggers_.clear();
}

void Animation::SetNumTriggers(unsigned num)
{
    triggers_.resize(num);
}

SharedPtr<Animation> Animation::Clone(const ea::string& cloneName) const
{
    SharedPtr<Animation> ret(context_->CreateObject<Animation>());

    ret->SetName(cloneName);
    ret->SetAnimationName(animationName_);
    ret->length_ = length_;
    ret->tracks_ = tracks_;
    ret->triggers_ = triggers_;
    ret->CopyMetadata(*this);
    ret->SetMemoryUse(GetMemoryUse());

    return ret;
}

AnimationTrack* Animation::GetTrack(unsigned index)
{
    if (index >= tracks_.size())
        return nullptr;

    const auto iter = ea::next(tracks_.begin(), index);
    return &iter->second;
}

AnimationTrack* Animation::GetTrack(const ea::string& name)
{
    auto i = tracks_.find(StringHash(name));
    return i != tracks_.end() ? &i->second : nullptr;
}

AnimationTrack* Animation::GetTrack(StringHash nameHash)
{
    auto i = tracks_.find(nameHash);
    return i != tracks_.end() ? &i->second : nullptr;
}

VariantAnimationTrack* Animation::GetVariantTrack(unsigned index)
{
    if (index >= variantTracks_.size())
        return nullptr;

    const auto iter = ea::next(variantTracks_.begin(), index);
    return &iter->second;
}

VariantAnimationTrack* Animation::GetVariantTrack(const ea::string& name)
{
    const auto iter = variantTracks_.find(StringHash(name));
    return iter != variantTracks_.end() ? &iter->second : nullptr;
}

VariantAnimationTrack* Animation::GetVariantTrack(StringHash nameHash)
{
    const auto iter = variantTracks_.find(nameHash);
    return iter != variantTracks_.end() ? &iter->second : nullptr;
}

AnimationTriggerPoint* Animation::GetTrigger(unsigned index)
{
    return index < triggers_.size() ? &triggers_[index] : nullptr;
}

void Animation::SetTracks(const ea::vector<AnimationTrack>& tracks)
{
    tracks_.clear();

    for (auto itr = tracks.begin(); itr != tracks.end(); itr++)
    {
        tracks_[itr->name_] = *itr;
    }
}

}
