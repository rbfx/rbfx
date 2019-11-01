%typemap(cscode) Urho3D::Engine %{
  public $typemap(cstype, const eastl::string &) AppPreferencesDir {
    get { return GetAppPreferencesDir(); }
  }
  public $typemap(cstype, float) NextTimeStep {
    get { return GetNextTimeStep(); }
    set { SetNextTimeStep(value); }
  }
  public $typemap(cstype, int) MinFps {
    get { return GetMinFps(); }
    set { SetMinFps(value); }
  }
  public $typemap(cstype, int) MaxFps {
    get { return GetMaxFps(); }
    set { SetMaxFps(value); }
  }
  public $typemap(cstype, int) MaxInactiveFps {
    get { return GetMaxInactiveFps(); }
    set { SetMaxInactiveFps(value); }
  }
  public $typemap(cstype, int) TimeStepSmoothing {
    get { return GetTimeStepSmoothing(); }
    set { SetTimeStepSmoothing(value); }
  }
  public $typemap(cstype, bool) PauseMinimized {
    get { return GetPauseMinimized(); }
    set { SetPauseMinimized(value); }
  }
  public $typemap(cstype, bool) AutoExit {
    get { return GetAutoExit(); }
    set { SetAutoExit(value); }
  }
%}
%csmethodmodifiers Urho3D::Engine::GetAppPreferencesDir "private";
%csmethodmodifiers Urho3D::Engine::GetNextTimeStep "private";
%csmethodmodifiers Urho3D::Engine::SetNextTimeStep "private";
%csmethodmodifiers Urho3D::Engine::GetMinFps "private";
%csmethodmodifiers Urho3D::Engine::SetMinFps "private";
%csmethodmodifiers Urho3D::Engine::GetMaxFps "private";
%csmethodmodifiers Urho3D::Engine::SetMaxFps "private";
%csmethodmodifiers Urho3D::Engine::GetMaxInactiveFps "private";
%csmethodmodifiers Urho3D::Engine::SetMaxInactiveFps "private";
%csmethodmodifiers Urho3D::Engine::GetTimeStepSmoothing "private";
%csmethodmodifiers Urho3D::Engine::SetTimeStepSmoothing "private";
%csmethodmodifiers Urho3D::Engine::GetPauseMinimized "private";
%csmethodmodifiers Urho3D::Engine::SetPauseMinimized "private";
%csmethodmodifiers Urho3D::Engine::GetAutoExit "private";
%csmethodmodifiers Urho3D::Engine::SetAutoExit "private";
