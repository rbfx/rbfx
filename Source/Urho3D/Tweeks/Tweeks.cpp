#include "../Tweeks/Tweeks.h"
#include "../Container/HashSet.h"
#include "../Core/Mutex.h"
#include "../Core/Object.h"
#include "../Core/Context.h"
#include "../Container/List.h"
#include "../Input/InputEvents.h"
#include "../UI/Cursor.h"
#include "../IO/Deserializer.h"
#include "../IO/Serializer.h"
#include "../Core/CoreEvents.h"

namespace Urho3D {

	Tweeks::Tweeks(Context* context) : Object(context)
	{
		BeginSection("default section");
		BeginTweekTime(TWEEK_LIFETIME_DEFAULT_MS);
	}

	Tweeks::~Tweeks()
	{

	}

	void Tweeks::RegisterObject(Context* context)
	{
		Tweek::RegisterObject(context);
		context->RegisterFactory<Tweeks>();


	}

	bool Tweeks::Save(Serializer* dest)
	{
		if (dest == nullptr)
			return false;

		dest->WriteInt(TWEEK_SERIALIZATION_VERSION);
		dest->WriteUInt(mTweekMap.Size());

		for (TweekMap::Iterator i = mTweekMap.Begin(); i != mTweekMap.End(); i++) {
			i->second_->Save(dest);
		}
		return true;
	}

	bool Tweeks::Load(Deserializer* source)
	{
		if (source == nullptr)
			return false;

		Clear();
		int serializationVersion = source->ReadInt();
		if (serializationVersion == TWEEK_SERIALIZATION_VERSION)
		{
			unsigned int mapSize = source->ReadUInt();
			for (unsigned int i = 0; i < mapSize; i++)
			{
				SharedPtr<Tweek> newTweek = context_->CreateObject<Tweek>();
				newTweek->Load(source);
				insertTweek(newTweek);
			}
		}
		return true;
	}


	TweekMap Tweeks::GetTweeks()
	{
		return mTweekMap;
	}

	Urho3D::TweekSectionMap Tweeks::GetTweekSectionMap()
	{
		return mTweekSectionMap;
	}

	Vector<SharedPtr<Tweek>> Tweeks::GetTweeksInSection(String section)
	{
		if (mTweekSectionMap.Contains(section))
			return mTweekSectionMap[section];
		else
			return Vector<SharedPtr<Tweek>>();
	}

	Urho3D::StringVector Tweeks::GetSections()
	{
		return mSections;
	}

	void Tweeks::BeginSection(String section)
	{
		mCurSectionStack.Push(section);
	}

	Urho3D::String Tweeks::CurrentSection()
	{
		return mCurSectionStack.Back();
	}

	void Tweeks::EndSection()
	{
		if (mCurSectionStack.Size() > 1)//dont pop the default section.
			mCurSectionStack.Pop();
	}

	void Tweeks::BeginTweekTime(unsigned int tweekLifeTimeMs)
	{
		mTweekTimeStack.Push(tweekLifeTimeMs);
	}

	unsigned int Tweeks::CurrentTweekTime()
	{
		return mTweekTimeStack.Back();
	}

	void Tweeks::EndTweekTime()
	{
		if (mTweekTimeStack.Size() > 1)
			mTweekTimeStack.Pop();
	}

	void Tweeks::Clear()
	{
		mTweekMap.Clear();
		mTweekSectionMap.Clear();
	}

	void Tweeks::TrimExpired()
	{
		Vector<StringHash> keys = mTweekMap.Keys();
		for (unsigned int i = 0; i < keys.Size(); i++) {
			Tweek* tw = mTweekMap[keys[i]];
			if (tw->IsExpired()) {
				mTweekMap.Erase(keys[i]);

				//remove from section map as well
				Vector<SharedPtr<Tweek>> tweeksInSection = mTweekSectionMap[tw->GetSection()];
				for (unsigned int j = 0; j < tweeksInSection.Size(); j++) {
					if (tweeksInSection[j]->GetName() == tw->GetName()) {
						mTweekSectionMap[tw->GetSection()].Erase(j);

						if (mTweekSectionMap[tw->GetSection()].Size() == 0) {
							mTweekSectionMap.Erase(tw->GetSection());
							mSections.Remove(tw->GetSection());
						}
					}
				}
			}
		}
	}



	Urho3D::Tweek* Tweeks::GetTweek(String name /*= ""*/, String section /*= ""*/)
	{			
		if (section.Empty())
				section = CurrentSection();//use the section stack if section is not specified.

		if (TweekExists(name, section))
		{
			Tweek* existingTweek = mTweekMap[name + section];
			existingTweek->ExtendLifeTime();
			return existingTweek;
		}
		else {


			SharedPtr<Tweek> newTweek = context_->CreateObject<Tweek>();
			newTweek->mExpirationTimer.SetTimeoutDuration(CurrentTweekTime());
			if (name.Empty()) {
				name = Tweek::GetTypeNameStatic();
			}

			newTweek->mName = (name);
			newTweek->mSection = (section);
			insertTweek(newTweek);
			return newTweek;
		}
	}

	bool Tweeks::TweekExists(String name, String section /*= ""*/)
	{
		if (section.Empty())
			section = CurrentSection();

		return mTweekMap.Contains(name + section);
	}

	void Tweeks::insertTweek(Tweek* tweek)
	{
		mTweekMap[tweekHash(tweek)] = SharedPtr<Tweek>(tweek);
		mTweekSectionMap[tweek->GetSection()].Push(SharedPtr<Tweek>(tweek));

		if (!mSections.Contains(tweek->GetSection())) {
			mSections.Push(tweek->GetSection());
		}
	}

	StringHash Tweeks::tweekHash(Tweek* tweek)
	{
		return tweek->GetName() + tweek->GetSection();
	}



	Tweek::Tweek(Context* context) : Object(context)
	{
	}

	Tweek::~Tweek()
	{

	}

	void Tweek::RegisterObject(Context* context)
	{
		context->RegisterFactory<Tweek>();
	}

	void Tweek::Save(Serializer* dest)
	{
		dest->WriteInt(TWEEK_SERIALIZATION_VERSION);
		dest->WriteString(mName);
		dest->WriteString(mSection);
		dest->WriteInt(mExpirationTimer.GetTimeoutDuration());
		dest->WriteVariant(mDefaultValue);
		dest->WriteVariant(mValue);
		dest->WriteVariant(mMinValue);
		dest->WriteVariant(mMaxValue);
	}

	void Tweek::Load(Deserializer* source)
	{
		int serializationVersion = source->ReadInt();

		if (serializationVersion == TWEEK_SERIALIZATION_VERSION) {
			mName = source->ReadString();
			mSection = source->ReadString();
			mExpirationTimer.SetTimeoutDuration(source->ReadInt());
			mDefaultValue = source->ReadVariant();
			mValue = source->ReadVariant();
			mMinValue = source->ReadVariant();
			mMaxValue = source->ReadVariant();
		}
	}


	int Tweek::GetTimeLeftMs()
	{
		return (mExpirationTimer.GetStartTime() + mExpirationTimer.GetTimeoutDuration()) - mExpirationTimer.GetMSec(false);
	}

	void Tweek::ExtendLifeTime()
	{
		mExpirationTimer.Reset();
	}





	String Tweek::GetSection()
	{
		return mSection;
	}

	String Tweek::GetName()
	{
		return mName;
	}


}