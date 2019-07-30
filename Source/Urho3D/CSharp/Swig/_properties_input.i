%typemap(cscode) Urho3D::Input %{
  public $typemap(cstype, Urho3D::Qualifier) Qualifiers {
    get { return GetQualifiers(); }
  }
  public $typemap(cstype, Urho3D::IntVector2) MousePosition {
    get { return GetMousePosition(); }
  }
  public $typemap(cstype, Urho3D::IntVector2) MouseMove {
    get { return GetMouseMove(); }
  }
  public $typemap(cstype, int) MouseMoveX {
    get { return GetMouseMoveX(); }
  }
  public $typemap(cstype, int) MouseMoveY {
    get { return GetMouseMoveY(); }
  }
  public $typemap(cstype, int) MouseMoveWheel {
    get { return GetMouseMoveWheel(); }
  }
  public $typemap(cstype, Urho3D::Vector2) InputScale {
    get { return GetInputScale(); }
  }
  public $typemap(cstype, unsigned int) NumTouches {
    get { return GetNumTouches(); }
  }
  public $typemap(cstype, unsigned int) NumJoysticks {
    get { return GetNumJoysticks(); }
  }
  public $typemap(cstype, bool) ToggleFullscreen {
    get { return GetToggleFullscreen(); }
    set { SetToggleFullscreen(value); }
  }
  public $typemap(cstype, bool) ScreenKeyboardSupport {
    get { return GetScreenKeyboardSupport(); }
  }
  public $typemap(cstype, bool) TouchEmulation {
    get { return GetTouchEmulation(); }
    set { SetTouchEmulation(value); }
  }
  public $typemap(cstype, Urho3D::MouseMode) MouseMode {
    get { return GetMouseMode(); }
  }
%}
%csmethodmodifiers Urho3D::Input::GetQualifiers "private";
%csmethodmodifiers Urho3D::Input::GetMousePosition "private";
%csmethodmodifiers Urho3D::Input::GetMouseMove "private";
%csmethodmodifiers Urho3D::Input::GetMouseMoveX "private";
%csmethodmodifiers Urho3D::Input::GetMouseMoveY "private";
%csmethodmodifiers Urho3D::Input::GetMouseMoveWheel "private";
%csmethodmodifiers Urho3D::Input::GetInputScale "private";
%csmethodmodifiers Urho3D::Input::GetNumTouches "private";
%csmethodmodifiers Urho3D::Input::GetNumJoysticks "private";
%csmethodmodifiers Urho3D::Input::GetToggleFullscreen "private";
%csmethodmodifiers Urho3D::Input::SetToggleFullscreen "private";
%csmethodmodifiers Urho3D::Input::GetScreenKeyboardSupport "private";
%csmethodmodifiers Urho3D::Input::GetTouchEmulation "private";
%csmethodmodifiers Urho3D::Input::SetTouchEmulation "private";
%csmethodmodifiers Urho3D::Input::GetMouseMode "private";
