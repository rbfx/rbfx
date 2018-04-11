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
#ifdef URHO3D_SYSTEMUI
#include "../SystemUI/SystemUI.h"
#endif
#include "../IO/File.h"
#include "../IO/FileSystem.h"

namespace Urho3D {

	Tweeks::Tweeks(Context* context) : Object(context)
	{
		mDefaultFileName.Resize(128);
		mCurrentSaveFileName = mDefaultFileName;
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

	bool Tweeks::Save(String filename)
	{
		SharedPtr<File> file = SharedPtr<File>(new File(context_, filename, FILE_WRITE));
		return Save((Serializer*)file);
	}

	bool Tweeks::Save()
	{
		return Save(mCurrentSaveFileName);
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


	bool Tweeks::Load(String filename)
	{
        mCurrentSaveFileName = filename;

		if (!GSS<FileSystem>()->FileExists(filename))
			return false;

		SharedPtr<File> file = SharedPtr<File>(new File(context_, filename, FILE_READ));
		bool s = Load((Deserializer*)file);

		return s;
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


#ifdef URHO3D_SYSTEMUI

	void Tweeks::RenderUIConsole()
	{


		//Tweeks
		//for each tweek in tweeks, make an approprate widget for alteration.
		Tweeks* twks = GetSubsystem<Tweeks>();
		Vector<SharedPtr<Tweek>> values = twks->GetTweeks().Values();
		ImGui::Begin("Tweeks");
	

		ImGui::InputText("File Name", (char*)mCurrentSaveFileName.CString(), mCurrentSaveFileName.Length());

		ImGui::SameLine(0, 10);
		if (ImGui::Button("Reset"))
		{
			mCurrentSaveFileName = mDefaultFileName;
		}

		if (ImGui::Button("Load")) {
			Load(mCurrentSaveFileName.Trimmed());
		}
		ImGui::SameLine(0, 10);
		if (ImGui::Button("Save")) {
			Save(mCurrentSaveFileName.Trimmed());
		}
		ImGui::SameLine(0, 10);
		if (ImGui::Button("Trim Expired")) {
			TrimExpired();
		}
		ImGui::SameLine(0, 10);
		if (ImGui::Button("Clear")) {
			twks->Clear();
		}



		TweekSectionMap tweekSectionMap = twks->GetTweekSectionMap();
		StringVector sections = twks->GetSections();

		for (auto section : sections) {
			if (ImGui::TreeNode(section.CString()))
			{
				Vector<SharedPtr<Tweek>> tweeksInSec = twks->GetTweeksInSection(section);
				for (auto tweek : tweeksInSec) {
					RenderTweekUI(tweek);
				}
				ImGui::TreePop();
			}
		}
		ImGui::End();


	}

	void Tweeks::RenderTweekUI(Tweek* tweek) {



		String name = tweek->GetName().CString();
		const char* tweekName = name.CString();
		VariantType type = tweek->mValue.GetType();
		bool tweekAltered = false;

		if (tweek->IsExpired())
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));

		switch (type)
		{
		case Urho3D::VAR_NONE:
			ImGui::Text("No Variant For Tweek: %s", tweekName);
			break;
		case Urho3D::VAR_INT: {
			int v = tweek->mValue.GetInt();
			ImGui::InputInt(tweekName, &v);

			if (tweek->mValue.GetInt() != v) {
				tweek->mValue = v;
				tweekAltered = true;
			}
			break;
		}
		case Urho3D::VAR_BOOL: {
			bool v = tweek->mValue.GetBool();
			ImGui::Checkbox(tweekName, &v);
			if (tweek->mValue.GetBool() != v) {
				tweek->mValue = v;
				tweekAltered = true;
			}
			break;
		}
		case Urho3D::VAR_FLOAT: {
			float v = tweek->mValue.GetFloat();
			ImGui::InputFloat(tweekName, &v);
			if (tweek->mValue.GetFloat() != v) {
				tweek->mValue = v;
				tweekAltered = true;
			}
			break;
		}
		case Urho3D::VAR_VECTOR2: {
			Vector2 v = tweek->mValue.GetVector2();
			float vals[2] = { v.x_, v.y_ };
			ImGui::InputFloat2(tweekName, vals);
			v = Vector2(vals[0], vals[1]);
			if (tweek->mValue.GetVector2() != v) {
				tweek->mValue = v;
				tweekAltered = true;
			}

			break;
		}
		case Urho3D::VAR_VECTOR3: {
			Vector3 v = tweek->mValue.GetVector3();
			float vals[3] = { v.x_, v.y_, v.z_ };
			ImGui::InputFloat3(tweekName, vals);
			v = Vector3(vals[0], vals[1], vals[2]);
			if (tweek->mValue.GetVector3() != v) {
				tweek->mValue = v;
				tweekAltered = true;
			}
			break;
		}
		case Urho3D::VAR_VECTOR4: {
			Vector4 v = tweek->mValue.GetVector4();
			float vals[4] = { v.x_, v.y_, v.z_, v.w_ };
			ImGui::InputFloat4(tweekName, vals);
			v = Vector4(vals[0], vals[1], vals[2], vals[3]);
			if (tweek->mValue.GetVector4() != v) {
				tweek->mValue = v;
				tweekAltered = true;
			}
			break;
		}
		case Urho3D::VAR_QUATERNION: {
			Quaternion v = tweek->mValue.GetQuaternion();
			float vals[4] = { v.x_, v.y_, v.z_, v.w_ };
			ImGui::InputFloat4(tweekName, vals);
			v = Quaternion(vals[3], vals[0], vals[1], vals[2]);
			if (tweek->mValue.GetQuaternion() != v) {
				tweek->mValue = v;
				tweekAltered = true;
			}
			break;
		}
		case Urho3D::VAR_COLOR: {
			Color v = tweek->mValue.GetColor();
			float vals[4] = { v.r_, v.g_, v.b_, v.a_ };
			ImGui::ColorEdit4(tweekName, vals);
			v = Color(vals[0], vals[1], vals[2], vals[3]);
			if (tweek->mValue.GetColor() != v) {
				tweek->mValue = v;
				tweekAltered = true;
			}
			break;
		}
		case Urho3D::VAR_STRING: {
			String v = tweek->mValue.GetString();

			//v.Resize(128);
			//v[v.Length() - 1] = '\0';
			//ImGui::InputText(tweekName, (char*)v.CString(), v.Length());
			//if (tweek->mValue.GetString() != v) {
			//	tweek->mValue = v;
			//	tweekAltered = true;
			//}
			//break;
		}
		case Urho3D::VAR_BUFFER:
			break;
		case Urho3D::VAR_VOIDPTR:
			break;
		case Urho3D::VAR_RESOURCEREF:
			break;
		case Urho3D::VAR_RESOURCEREFLIST:
			break;
		case Urho3D::VAR_VARIANTVECTOR:
			break;
		case Urho3D::VAR_VARIANTMAP:
			break;
		case Urho3D::VAR_INTRECT: {
			IntRect v = tweek->mValue.GetIntRect();
			int vals[4] = { v.left_, v.top_, v.right_, v.bottom_ };
			ImGui::InputInt4(tweekName, vals);
			v = IntRect(IntVector2(vals[0], vals[1]), IntVector2(vals[2], vals[3]));
			if (tweek->mValue.GetIntRect() != v) {
				tweek->mValue = v;
				tweekAltered = true;
			}
			break;
		}
		case Urho3D::VAR_INTVECTOR2: {
			IntVector2 v = tweek->mValue.GetIntVector2();
			int vals[2] = { v.x_, v.y_ };
			ImGui::InputInt2(tweekName, vals);
			v = IntVector2(vals[0], vals[1]);
			if (tweek->mValue.GetIntVector2() != v) {
				tweek->mValue = v;
				tweekAltered = true;
			}
			break;
		}
		case Urho3D::VAR_PTR:
			break;
		case Urho3D::VAR_MATRIX3:
			break;
		case Urho3D::VAR_MATRIX3X4:
			break;
		case Urho3D::VAR_MATRIX4:
			break;
		case Urho3D::VAR_DOUBLE: {
			float v = tweek->mValue.GetDouble();
			ImGui::InputFloat(tweekName, &v);
			if (tweek->mValue.GetDouble() != v) {
				tweek->mValue = v;
				tweekAltered = true;
			}
			break;
		}
		case Urho3D::VAR_STRINGVECTOR:
			break;
		case Urho3D::VAR_RECT: {
			Rect v = tweek->mValue.GetRect();
			float vals[4] = { v.min_.x_, v.min_.y_, v.max_.x_, v.max_.y_ };
			ImGui::InputFloat4(tweekName, vals);
			v = Rect(Vector2(vals[0], vals[1]), Vector2(vals[2], vals[3]));
			if (tweek->mValue.GetRect() != v) {
				tweek->mValue = v;
				tweekAltered = true;
			}
			break;
		}
		case Urho3D::VAR_INTVECTOR3: {
			IntVector3 v = tweek->mValue.GetIntVector3();
			int vals[3] = { v.x_, v.y_, v.z_ };
			ImGui::InputInt3(tweekName, vals);
			v = IntVector3(vals[0], vals[1], vals[2]);
			if (tweek->mValue.GetIntVector3() != v) {
				tweek->mValue = v;
				tweekAltered = true;
			}
			break;
		}
		case Urho3D::VAR_INT64: {
			int v = tweek->mValue.GetInt();
			ImGui::InputInt(tweekName, &v);
			if (tweek->mValue.GetInt() != v) {
				tweek->mValue = v;
				tweekAltered = true;
			}
			break;
		}
		case Urho3D::VAR_CUSTOM_HEAP:
			break;
		case Urho3D::VAR_CUSTOM_STACK:
			break;
		case Urho3D::MAX_VAR_TYPES:
			break;
		default:
			break;
		}

		if (!tweek->IsDefaultValue()) {
			ImGui::SameLine(0.0f, 10.0f);
			if (ImGui::Button("Reset To Default"))
				tweek->RevertToDefaultValue();

			if (type == VAR_QUATERNION) {
				ImGui::SameLine(0.0f, 10.0f);
				if (ImGui::Button("Normalize"))
					tweek->mValue = tweek->mValue.GetQuaternion().Normalized();
			}
		}

		if (tweek->IsExpired())
			ImGui::PopStyleColor();
	}
#endif








































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
