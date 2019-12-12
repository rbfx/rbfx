%typemap(cscode) Urho3D::Console %{
  public $typemap(cstype, const eastl::string &) CommandInterpreter {
    get { return GetCommandInterpreter(); }
    set { SetCommandInterpreter(value); }
  }
  public $typemap(cstype, unsigned int) NumHistoryRows {
    get { return GetNumHistoryRows(); }
    set { SetNumHistoryRows(value); }
  }
  public $typemap(cstype, eastl::vector<eastl::string>) Loggers {
    get { return GetLoggers(); }
  }
%}
%csmethodmodifiers Urho3D::Console::GetCommandInterpreter "private";
%csmethodmodifiers Urho3D::Console::SetCommandInterpreter "private";
%csmethodmodifiers Urho3D::Console::GetNumHistoryRows "private";
%csmethodmodifiers Urho3D::Console::SetNumHistoryRows "private";
%csmethodmodifiers Urho3D::Console::GetLoggers "private";
%typemap(cscode) Urho3D::DebugHud %{
  public $typemap(cstype, Urho3D::DebugHudMode) Mode {
    get { return GetMode(); }
    set { SetMode(value); }
  }
  public $typemap(cstype, bool) UseRendererStats {
    get { return GetUseRendererStats(); }
    set { SetUseRendererStats(value); }
  }
%}
%csmethodmodifiers Urho3D::DebugHud::GetMode "private";
%csmethodmodifiers Urho3D::DebugHud::SetMode "private";
%csmethodmodifiers Urho3D::DebugHud::GetUseRendererStats "private";
%csmethodmodifiers Urho3D::DebugHud::SetUseRendererStats "private";
%typemap(cscode) Urho3D::SystemMessageBox %{
  public $typemap(cstype, const eastl::string &) Title {
    get { return GetTitle(); }
    set { SetTitle(value); }
  }
  public $typemap(cstype, const eastl::string &) Message {
    get { return GetMessage(); }
    set { SetMessage(value); }
  }
%}
%csmethodmodifiers Urho3D::SystemMessageBox::GetTitle "private";
%csmethodmodifiers Urho3D::SystemMessageBox::SetTitle "private";
%csmethodmodifiers Urho3D::SystemMessageBox::GetMessage "private";
%csmethodmodifiers Urho3D::SystemMessageBox::SetMessage "private";
