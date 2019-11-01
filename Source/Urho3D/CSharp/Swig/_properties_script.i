%typemap(cscode) Urho3D::GCHandleRef %{
  public $typemap(cstype, void *) Handle {
    get { return GetHandle(); }
  }
%}
%csmethodmodifiers Urho3D::GCHandleRef::GetHandle "private";
