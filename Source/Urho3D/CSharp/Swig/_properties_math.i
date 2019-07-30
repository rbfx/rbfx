%typemap(cscode) Urho3D::AreaAllocator %{
  public $typemap(cstype, int) Width {
    get { return GetWidth(); }
  }
  public $typemap(cstype, int) Height {
    get { return GetHeight(); }
  }
  public $typemap(cstype, bool) FastMode {
    get { return GetFastMode(); }
  }
%}
%csmethodmodifiers Urho3D::AreaAllocator::GetWidth "private";
%csmethodmodifiers Urho3D::AreaAllocator::GetHeight "private";
%csmethodmodifiers Urho3D::AreaAllocator::GetFastMode "private";
