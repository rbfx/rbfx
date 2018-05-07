#pragma once
#include "../Core/Object.h"
#include "../Core/Context.h"
#include "../Core/Timer.h"


namespace Urho3D
{
	#define TWEEK_LIFETIME_DEFAULT_MS 2000 //2 seconds
	#define TWEEK_SERIALIZATION_VERSION 0

	class Serializer;
	class Deserializer;


	//encapsulates a single tweekable value.
	class URHO3D_API Tweek : public Object {

		URHO3D_OBJECT(Tweek, Object);

	public: 
		friend class Tweeks;

	public:
		explicit Tweek(Context* context);
		virtual ~Tweek();
	
		static void RegisterObject(Context* context);

		void Save(Serializer* dest);
		void Load(Deserializer* source);

		//returns approximately how many milliseconds until this tweek is marked expired.
		int GetTimeLeftMs();

		//sets the lifetime in milliseconds of the tweek since it was created. 0 will make the tweek never expire.
		void SetLifetimeMs(unsigned int lifeTimeMs) { mExpirationTimer.SetTimeoutDuration(lifeTimeMs); }

		//returns the lifetime of the tweek in milliseconds since it was created.
		unsigned int GetLifetimeMs() { return mExpirationTimer.GetTimeoutDuration(); }

		//extends the lifetime of the tweek by reseting its expiration timer.
		void ExtendLifeTime();

		//returns true if the tweek is expired.
		bool IsExpired() { return mExpirationTimer.IsTimedOut(); }

		//returns the name of the section the tweek belongs to.
		String GetSection();

		//returns the name of the tweek.
		String GetName();

		//reverts the value to the default value defined in code by the first Get/Update call.
		void RevertToDefaultValue() {
			mValue = mDefaultValue;
		}

		//returns true if the current value is the same as the default value.
		bool IsDefaultValue() {
			VariantType valType = mValue.GetType();
			VariantType defType = mDefaultValue.GetType();

			if (valType != defType)
				return false;

			//handle fuzzy comparisons
			switch (valType)
			{
			case Urho3D::VAR_QUATERNION:
				return mValue.GetQuaternion().Equals(mDefaultValue.GetQuaternion());
				break;
			case Urho3D::VAR_COLOR:
				return mValue.GetColor().Equals(mDefaultValue.GetColor());
				break;

			default:
				break;
			}

			return (mValue == mDefaultValue);
		}


		//optional max value
		Variant mMaxValue;
		
		//optional min value
		Variant mMinValue;

		//the value of the tweek.
		Variant mValue;

		//the original default value defined through the Get/Update functions.
		Variant mDefaultValue;


	protected:

		String mName;
		String mSection;

		Timer mExpirationTimer;
	};

	typedef HashMap<StringHash, SharedPtr<Tweek>> TweekMap;
	typedef HashMap<StringHash, Vector<SharedPtr<Tweek>>> TweekSectionMap;

	//subsystem designed to add easy/FAST access to simple value types that can be easily tweeked by an overlay gui as well as saved/loaded from a config file.
	class URHO3D_API Tweeks : public Object {

		URHO3D_OBJECT(Tweeks, Object);

	public: 
		friend class Tweek;

	public:
		explicit Tweeks(Context* context);
		virtual ~Tweeks();

		static void RegisterObject(Context* context);
		/// Saves all tweeks to dest
		bool Save(Serializer* dest);
		/// Saves all tweeks to filename
		bool Save(String filename);
		/// Saves all tweeks to last loaded filename.
		bool Save();
		
		/// clears all tweeks and Loads source.
		bool Load(Deserializer* source);
		/// clears all tweeks and Loads filename.
		bool Load(String filename);

		/// returns the entire map of tweeks
		TweekMap GetTweeks();

		/// returns map of tweeks with easily lookup by section
		TweekSectionMap GetTweekSectionMap();

		/// returns a list of tweeks in a section
		Vector<SharedPtr<Tweek>> GetTweeksInSection(String section);

		/// returns the names of all sections
		StringVector GetSections();

		/// start a new section
		void BeginSection(String section);

		/// returns the current section.
		String CurrentSection();

		/// ends the current section restoring the previous section.
		void EndSection();

		/// starts a new default tweek lifetime.
		void BeginTweekTime(unsigned int tweekLifeTimeMs);

		/// returns the current default tweek time.
		unsigned int CurrentTweekTime();

		/// ends the current tweek time - restoring the previous tweek time.
		void EndTweekTime();

		/// clears all tweeks.
		void Clear();

		/// iterates through all tweeks and removes the tweeks that have expired.
		void TrimExpired();


		/// returns a new or existing tweek.
		Tweek* GetTweek(String name, String section = "");
		
		bool TweekExists(String name, String section = "");

		template <typename T>
		T GetDefault(String name, T defaultVal = T(), String section = "", Tweek** tweek_out = nullptr) {
			Tweek* tw = nullptr;
			if (TweekExists(name, section))
			{
				tw = GetTweek(name, section);
			}
			else {
				tw = Update(name, defaultVal, section);
			}

			//update the reference.
			if (tweek_out != nullptr)
				*tweek_out = tw;

			return tw->mValue.Get<T>();
		}

		template <typename T>
		T Get(String name, String section = "", Tweek** tweek_out = nullptr) {
			return GetDefault<T>(name, T(), section, tweek_out);
		}

		/// updates a tweek with value, will create a new tweek if needed.
		template <typename T>
		Tweek* Update(String name, T value, String section = "") {
			Tweek* tw = nullptr;
			if (TweekExists(name, section)) {
				tw = GetTweek(name, section);
				tw->mValue = value;

			}
			else
			{
				tw = GetTweek(name, section);
				tw->mValue = value;
				tw->mDefaultValue = value;
			}
			return tw;
		}

#ifdef URHO3D_SYSTEMUI

		/// shows the ui console on imgui
		void RenderUIConsole();

		/// renders a single tweek on imgui
		void RenderTweekUI(Tweek* tweek);
#endif

	protected:

		void insertTweek(Tweek* tweek);
		StringHash tweekHash(Tweek* tweek);

		TweekMap mTweekMap; //tweek lookup by name
		TweekSectionMap mTweekSectionMap;//lookup list of tweeks by section
		StringVector mSections;
		String mDefaultFileName = "Tweeks.twks";
		String mCurrentSaveFileName;

		StringVector mCurSectionStack;
		Vector<unsigned int> mTweekTimeStack;
	};


}