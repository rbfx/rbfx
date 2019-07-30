%typemap(cscode) Urho3D::Audio %{
  public $typemap(cstype, unsigned int) SampleSize {
    get { return GetSampleSize(); }
  }
  public $typemap(cstype, int) MixRate {
    get { return GetMixRate(); }
  }
  public $typemap(cstype, bool) Interpolation {
    get { return GetInterpolation(); }
  }
  public $typemap(cstype, Urho3D::SoundListener *) Listener {
    get { return GetListener(); }
    set { SetListener(value); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::SoundSource *> &) SoundSources {
    get { return GetSoundSources(); }
  }
  public $typemap(cstype, Urho3D::Mutex &) Mutex {
    get { return GetMutex(); }
  }
%}
%csmethodmodifiers Urho3D::Audio::GetSampleSize "private";
%csmethodmodifiers Urho3D::Audio::GetMixRate "private";
%csmethodmodifiers Urho3D::Audio::GetInterpolation "private";
%csmethodmodifiers Urho3D::Audio::GetListener "private";
%csmethodmodifiers Urho3D::Audio::SetListener "private";
%csmethodmodifiers Urho3D::Audio::GetSoundSources "private";
%csmethodmodifiers Urho3D::Audio::GetMutex "private";
%typemap(cscode) Urho3D::BufferedSoundStream %{
  public $typemap(cstype, unsigned int) BufferNumBytes {
    get { return GetBufferNumBytes(); }
  }
  public $typemap(cstype, float) BufferLength {
    get { return GetBufferLength(); }
  }
%}
%csmethodmodifiers Urho3D::BufferedSoundStream::GetBufferNumBytes "private";
%csmethodmodifiers Urho3D::BufferedSoundStream::GetBufferLength "private";
%typemap(cscode) Urho3D::Sound %{
  public $typemap(cstype, Urho3D::SharedPtr<Urho3D::SoundStream>) DecoderStream {
    get { return GetDecoderStream(); }
  }
  /*public _typemap(cstype, eastl::shared_array<signed char>) Data {
    get { return GetData(); }
  }*/
  public $typemap(cstype, signed char *) Start {
    get { return GetStart(); }
  }
  public $typemap(cstype, signed char *) Repeat {
    get { return GetRepeat(); }
  }
  public $typemap(cstype, signed char *) End {
    get { return GetEnd(); }
  }
  public $typemap(cstype, float) Length {
    get { return GetLength(); }
  }
  public $typemap(cstype, unsigned int) DataSize {
    get { return GetDataSize(); }
  }
  public $typemap(cstype, unsigned int) SampleSize {
    get { return GetSampleSize(); }
  }
  public $typemap(cstype, float) Frequency {
    get { return GetFrequency(); }
  }
  public $typemap(cstype, unsigned int) IntFrequency {
    get { return GetIntFrequency(); }
  }
%}
%csmethodmodifiers Urho3D::Sound::GetDecoderStream "private";
%csmethodmodifiers Urho3D::Sound::GetData "private";
%csmethodmodifiers Urho3D::Sound::GetStart "private";
%csmethodmodifiers Urho3D::Sound::GetRepeat "private";
%csmethodmodifiers Urho3D::Sound::GetEnd "private";
%csmethodmodifiers Urho3D::Sound::GetLength "private";
%csmethodmodifiers Urho3D::Sound::GetDataSize "private";
%csmethodmodifiers Urho3D::Sound::GetSampleSize "private";
%csmethodmodifiers Urho3D::Sound::GetFrequency "private";
%csmethodmodifiers Urho3D::Sound::GetIntFrequency "private";
%typemap(cscode) Urho3D::SoundSource %{
  public $typemap(cstype, Urho3D::Sound *) Sound {
    get { return GetSound(); }
  }
  public $typemap(cstype, volatile signed char *) PlayPosition {
    get { return GetPlayPosition(); }
  }
  public $typemap(cstype, eastl::string) SoundType {
    get { return GetSoundType(); }
  }
  public $typemap(cstype, float) TimePosition {
    get { return GetTimePosition(); }
  }
  public $typemap(cstype, float) Frequency {
    get { return GetFrequency(); }
    set { SetFrequency(value); }
  }
  public $typemap(cstype, float) Gain {
    get { return GetGain(); }
    set { SetGain(value); }
  }
  public $typemap(cstype, float) Attenuation {
    get { return GetAttenuation(); }
    set { SetAttenuation(value); }
  }
  public $typemap(cstype, float) Panning {
    get { return GetPanning(); }
    set { SetPanning(value); }
  }
  public $typemap(cstype, Urho3D::AutoRemoveMode) AutoRemoveMode {
    get { return GetAutoRemoveMode(); }
    set { SetAutoRemoveMode(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) SoundAttr {
    get { return GetSoundAttr(); }
  }
  public $typemap(cstype, int) PositionAttr {
    get { return GetPositionAttr(); }
    set { SetPositionAttr(value); }
  }
%}
%csmethodmodifiers Urho3D::SoundSource::GetSound "private";
%csmethodmodifiers Urho3D::SoundSource::GetPlayPosition "private";
%csmethodmodifiers Urho3D::SoundSource::GetSoundType "private";
%csmethodmodifiers Urho3D::SoundSource::GetTimePosition "private";
%csmethodmodifiers Urho3D::SoundSource::GetFrequency "private";
%csmethodmodifiers Urho3D::SoundSource::SetFrequency "private";
%csmethodmodifiers Urho3D::SoundSource::GetGain "private";
%csmethodmodifiers Urho3D::SoundSource::SetGain "private";
%csmethodmodifiers Urho3D::SoundSource::GetAttenuation "private";
%csmethodmodifiers Urho3D::SoundSource::SetAttenuation "private";
%csmethodmodifiers Urho3D::SoundSource::GetPanning "private";
%csmethodmodifiers Urho3D::SoundSource::SetPanning "private";
%csmethodmodifiers Urho3D::SoundSource::GetAutoRemoveMode "private";
%csmethodmodifiers Urho3D::SoundSource::SetAutoRemoveMode "private";
%csmethodmodifiers Urho3D::SoundSource::GetSoundAttr "private";
%csmethodmodifiers Urho3D::SoundSource::GetPositionAttr "private";
%csmethodmodifiers Urho3D::SoundSource::SetPositionAttr "private";
%typemap(cscode) Urho3D::SoundSource3D %{
  public $typemap(cstype, float) NearDistance {
    get { return GetNearDistance(); }
    set { SetNearDistance(value); }
  }
  public $typemap(cstype, float) FarDistance {
    get { return GetFarDistance(); }
    set { SetFarDistance(value); }
  }
  public $typemap(cstype, float) InnerAngle {
    get { return GetInnerAngle(); }
    set { SetInnerAngle(value); }
  }
  public $typemap(cstype, float) OuterAngle {
    get { return GetOuterAngle(); }
    set { SetOuterAngle(value); }
  }
%}
%csmethodmodifiers Urho3D::SoundSource3D::GetNearDistance "private";
%csmethodmodifiers Urho3D::SoundSource3D::SetNearDistance "private";
%csmethodmodifiers Urho3D::SoundSource3D::GetFarDistance "private";
%csmethodmodifiers Urho3D::SoundSource3D::SetFarDistance "private";
%csmethodmodifiers Urho3D::SoundSource3D::GetInnerAngle "private";
%csmethodmodifiers Urho3D::SoundSource3D::SetInnerAngle "private";
%csmethodmodifiers Urho3D::SoundSource3D::GetOuterAngle "private";
%csmethodmodifiers Urho3D::SoundSource3D::SetOuterAngle "private";
%typemap(cscode) Urho3D::SoundStream %{
  public $typemap(cstype, unsigned int) SampleSize {
    get { return GetSampleSize(); }
  }
  public $typemap(cstype, float) Frequency {
    get { return GetFrequency(); }
  }
  public $typemap(cstype, unsigned int) IntFrequency {
    get { return GetIntFrequency(); }
  }
  public $typemap(cstype, bool) StopAtEnd {
    get { return GetStopAtEnd(); }
    set { SetStopAtEnd(value); }
  }
%}
%csmethodmodifiers Urho3D::SoundStream::GetSampleSize "private";
%csmethodmodifiers Urho3D::SoundStream::GetFrequency "private";
%csmethodmodifiers Urho3D::SoundStream::GetIntFrequency "private";
%csmethodmodifiers Urho3D::SoundStream::GetStopAtEnd "private";
%csmethodmodifiers Urho3D::SoundStream::SetStopAtEnd "private";
