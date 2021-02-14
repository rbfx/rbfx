%constant ea::string SoundMaster = Urho3D::SOUND_MASTER;
%ignore Urho3D::SOUND_MASTER;
%constant ea::string SoundEffect = Urho3D::SOUND_EFFECT;
%ignore Urho3D::SOUND_EFFECT;
%constant ea::string SoundAmbient = Urho3D::SOUND_AMBIENT;
%ignore Urho3D::SOUND_AMBIENT;
%constant ea::string SoundVoice = Urho3D::SOUND_VOICE;
%ignore Urho3D::SOUND_VOICE;
%constant ea::string SoundMusic = Urho3D::SOUND_MUSIC;
%ignore Urho3D::SOUND_MUSIC;
%constant int StreamBufferLength = Urho3D::STREAM_BUFFER_LENGTH;
%ignore Urho3D::STREAM_BUFFER_LENGTH;
%csattribute(Urho3D::Audio, %arg(unsigned int), SampleSize, GetSampleSize);
%csattribute(Urho3D::Audio, %arg(int), MixRate, GetMixRate);
%csattribute(Urho3D::Audio, %arg(bool), Interpolation, GetInterpolation);
%csattribute(Urho3D::Audio, %arg(bool), IsStereo, IsStereo);
%csattribute(Urho3D::Audio, %arg(bool), IsPlaying, IsPlaying);
%csattribute(Urho3D::Audio, %arg(bool), IsInitialized, IsInitialized);
%csattribute(Urho3D::Audio, %arg(Urho3D::SoundListener *), Listener, GetListener, SetListener);
%csattribute(Urho3D::Audio, %arg(ea::vector<SoundSource *>), SoundSources, GetSoundSources);
%csattribute(Urho3D::Audio, %arg(Urho3D::Mutex), Mutex, GetMutex);
%csattribute(Urho3D::SoundStream, %arg(unsigned int), SampleSize, GetSampleSize);
%csattribute(Urho3D::SoundStream, %arg(float), Frequency, GetFrequency);
%csattribute(Urho3D::SoundStream, %arg(unsigned int), IntFrequency, GetIntFrequency);
%csattribute(Urho3D::SoundStream, %arg(bool), StopAtEnd, GetStopAtEnd, SetStopAtEnd);
%csattribute(Urho3D::SoundStream, %arg(bool), IsSixteenBit, IsSixteenBit);
%csattribute(Urho3D::SoundStream, %arg(bool), IsStereo, IsStereo);
%csattribute(Urho3D::BufferedSoundStream, %arg(unsigned int), BufferNumBytes, GetBufferNumBytes);
%csattribute(Urho3D::BufferedSoundStream, %arg(float), BufferLength, GetBufferLength);
%csattribute(Urho3D::Sound, %arg(SharedPtr<Urho3D::SoundStream>), DecoderStream, GetDecoderStream);
%csattribute(Urho3D::Sound, %arg(ea::shared_array<signed char>), Data, GetData);
%csattribute(Urho3D::Sound, %arg(signed char *), Start, GetStart);
%csattribute(Urho3D::Sound, %arg(signed char *), Repeat, GetRepeat);
%csattribute(Urho3D::Sound, %arg(signed char *), End, GetEnd);
%csattribute(Urho3D::Sound, %arg(float), Length, GetLength);
%csattribute(Urho3D::Sound, %arg(unsigned int), DataSize, GetDataSize);
%csattribute(Urho3D::Sound, %arg(unsigned int), SampleSize, GetSampleSize);
%csattribute(Urho3D::Sound, %arg(float), Frequency, GetFrequency);
%csattribute(Urho3D::Sound, %arg(unsigned int), IntFrequency, GetIntFrequency);
%csattribute(Urho3D::Sound, %arg(bool), IsLooped, IsLooped, SetLooped);
%csattribute(Urho3D::Sound, %arg(bool), IsSixteenBit, IsSixteenBit);
%csattribute(Urho3D::Sound, %arg(bool), IsStereo, IsStereo);
%csattribute(Urho3D::Sound, %arg(bool), IsCompressed, IsCompressed);
%csattribute(Urho3D::SoundSource, %arg(Urho3D::Sound *), Sound, GetSound);
%csattribute(Urho3D::SoundSource, %arg(volatile signed char *), PlayPosition, GetPlayPosition);
%csattribute(Urho3D::SoundSource, %arg(ea::string), SoundType, GetSoundType, SetSoundType);
%csattribute(Urho3D::SoundSource, %arg(float), TimePosition, GetTimePosition);
%csattribute(Urho3D::SoundSource, %arg(float), Frequency, GetFrequency, SetFrequency);
%csattribute(Urho3D::SoundSource, %arg(float), Gain, GetGain, SetGain);
%csattribute(Urho3D::SoundSource, %arg(float), Attenuation, GetAttenuation, SetAttenuation);
%csattribute(Urho3D::SoundSource, %arg(float), Panning, GetPanning, SetPanning);
%csattribute(Urho3D::SoundSource, %arg(Urho3D::AutoRemoveMode), AutoRemoveMode, GetAutoRemoveMode, SetAutoRemoveMode);
%csattribute(Urho3D::SoundSource, %arg(bool), IsPlaying, IsPlaying);
%csattribute(Urho3D::SoundSource, %arg(Urho3D::ResourceRef), SoundAttr, GetSoundAttr, SetSoundAttr);
%csattribute(Urho3D::SoundSource, %arg(int), PositionAttr, GetPositionAttr, SetPositionAttr);
%csattribute(Urho3D::SoundSource3D, %arg(float), NearDistance, GetNearDistance, SetNearDistance);
%csattribute(Urho3D::SoundSource3D, %arg(float), FarDistance, GetFarDistance, SetFarDistance);
%csattribute(Urho3D::SoundSource3D, %arg(float), InnerAngle, GetInnerAngle, SetInnerAngle);
%csattribute(Urho3D::SoundSource3D, %arg(float), OuterAngle, GetOuterAngle, SetOuterAngle);
%pragma(csharp) moduleimports=%{
public static partial class E
{
    public class SoundFinishedEvent {
        private StringHash _event = new StringHash("SoundFinished");
        public StringHash Node = new StringHash("Node");
        public StringHash SoundSource = new StringHash("SoundSource");
        public StringHash Sound = new StringHash("Sound");
        public SoundFinishedEvent() { }
        public static implicit operator StringHash(SoundFinishedEvent e) { return e._event; }
    }
    public static SoundFinishedEvent SoundFinished = new SoundFinishedEvent();
} %}
