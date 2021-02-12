
// Proxy classes (base classes, ie, not derived classes)
%typemap(csbody) SWIGTYPE %{
  [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
  private bool swigCMemOwn;
  [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
  private static global::Urho3DNet.InstanceCache<$csclassname> _instanceCache = new global::Urho3DNet.InstanceCache<$csclassname>();
  internal static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn)
  {
    if (cPtr == global::System.IntPtr.Zero)
      return null;
    return _instanceCache.GetOrAdd(cPtr, () => {
      global::System.Type type;
      if (!$imclassname.SWIGTypeRegistry.TryGetValue($imclassname.$csclazznameSWIGTypeId(cPtr), out type))
        type = typeof($csclassname);
      if (type == typeof($csclassname))
        return new $csclassname(cPtr, cMemoryOwn);
      return ($csclassname)global::System.Activator.CreateInstance(type, global::System.Reflection.BindingFlags.Instance|global::System.Reflection.BindingFlags.NonPublic|global::System.Reflection.BindingFlags.Public, null, new object[]{cPtr, cMemoryOwn}, null);
    });
  }

  internal $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
    _instanceCache.AddNew(swigCPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }
%}

// Derived proxy classes
%typemap(csbody_derived, directorsetup="\n    SetupSwigDirector();") SWIGTYPE %{
  [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
  private bool swigCMemOwn;
  [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
  private static global::Urho3DNet.InstanceCache<$csclassname> _instanceCache = new global::Urho3DNet.InstanceCache<$csclassname>();
  internal new static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn)
  {
    if (cPtr == global::System.IntPtr.Zero)
      return null;
    return _instanceCache.GetOrAdd(cPtr, () => {
      global::System.Type type;
      if (!$imclassname.SWIGTypeRegistry.TryGetValue($imclassname.$csclazznameSWIGTypeId(cPtr), out type))
        type = typeof($csclassname);
      if (type == typeof($csclassname))
        return new $csclassname(cPtr, cMemoryOwn);
      return ($csclassname)global::System.Activator.CreateInstance(type, global::System.Reflection.BindingFlags.Instance|global::System.Reflection.BindingFlags.NonPublic|global::System.Reflection.BindingFlags.Public, null, new object[]{cPtr, cMemoryOwn}, null);
    });
  }

  internal $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn) : base($imclassname.$csclazznameSWIGUpcast(cPtr), false) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);$directorsetup
    _instanceCache.AddNew(swigCPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }
%}

%typemap(csdisposing, methodname="Dispose", methodmodifiers="protected", parameters="bool disposing") SWIGTYPE {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        _instanceCache.Remove(swigCPtr);
        if (swigCMemOwn) {
          swigCMemOwn = false;
          $imcall;
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
    }
  }

%typemap(csdisposing_derived, methodname="Dispose", methodmodifiers="protected", parameters="bool disposing") SWIGTYPE {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        _instanceCache.Remove(swigCPtr);
        if (swigCMemOwn) {
          swigCMemOwn = false;
          $imcall;
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      base.Dispose(disposing);
    }
  }

%typemap(csdirectorin) SWIGTYPE "$&csclassname.wrap($iminput, true)"
%typemap(csdirectorin) SWIGTYPE *, SWIGTYPE (CLASS::*) "($iminput == global::System.IntPtr.Zero) ? null : global::$nspace.$csclassname.wrap($iminput, false)"
%typemap(csdirectorin) SWIGTYPE & "$csclassname.wrap($iminput, false)"
%typemap(csdirectorin) SWIGTYPE && "$csclassname.wrap($iminput, false)"

%typemap(csout, excode=SWIGEXCODE) SWIGTYPE {
    $&csclassname ret = global::$nspace.$&csclassname.wrap($imcall, true);$excode
    return ret;
  }
%typemap(csout, excode=SWIGEXCODE) SWIGTYPE & {
    $csclassname ret = global::$nspace.$csclassname.wrap($imcall, $owner);$excode
    return ret;
  }
%typemap(csout, excode=SWIGEXCODE) SWIGTYPE && {
    $csclassname ret = global::$nspace.$csclassname.wrap($imcall, $owner);$excode
    return ret;
  }
%typemap(csout, excode=SWIGEXCODE) SWIGTYPE *, SWIGTYPE [] {
    global::System.IntPtr cPtr = $imcall;
    $csclassname ret = (cPtr == global::System.IntPtr.Zero) ? null : global::$nspace.$csclassname.wrap(cPtr, $owner);$excode
    return ret;
  }
%typemap(csout, excode=SWIGEXCODE) SWIGTYPE (CLASS::*) {
    string cMemberPtr = $imcall;
    $csclassname ret = (cMemberPtr == null) ? null : global::$nspace.$csclassname.wrap(cMemberPtr, $owner);$excode
    return ret;
  }
%typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE %{
    get {
      $&csclassname ret = global::$nspace.$&csclassname.wrap($imcall, true);$excode
      return ret;
    } %}
%typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE & %{
    get {
      $csclassname ret = global::$nspace.$csclassname.wrap($imcall, $owner);$excode
      return ret;
    } %}
%typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE && %{
    get {
      $csclassname ret = global::$nspace.$csclassname.wrap($imcall, $owner);$excode
      return ret;
    } %}
%typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE *, SWIGTYPE [] %{
    get {
      global::System.IntPtr cPtr = $imcall;
      $csclassname ret = (cPtr == global::System.IntPtr.Zero) ? null : global::$nspace.$csclassname.wrap(cPtr, $owner);$excode
      return ret;
    } %}

%typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE (CLASS::*) %{
    get {
      string cMemberPtr = $imcall;
      $csclassname ret = (cMemberPtr == null) ? null : global::$nspace.$csclassname.wrap(cMemberPtr, $owner);$excode
      return ret;
    } %}
%typemap(csout, excode=SWIGEXCODE) SWIGTYPE *const& {
    global::System.IntPtr cPtr = $imcall;
    $*csclassname ret = (cPtr == global::System.IntPtr.Zero) ? null : global::$nspace.$*csclassname.wrap(cPtr, $owner);$excode
    return ret;
  }

%typemap(csbody) SWIGTYPE *, SWIGTYPE &, SWIGTYPE &&, SWIGTYPE [] %{
  [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
  private static global::Urho3DNet.InstanceCache<$csclassname> _instanceCache = new global::Urho3DNet.InstanceCache<$csclassname>();
  internal static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn)
  {
    if (cPtr == global::System.IntPtr.Zero)
      return null;
    return _instanceCache.GetOrAdd(cPtr, () => new $csclassname(cPtr, cMemoryOwn));
  }

  internal $csclassname(global::System.IntPtr cPtr, bool futureUse) {
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
    _instanceCache.AddNew(swigCPtr);
  }

  protected $csclassname() {
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }
%}

%pragma(csharp) imclasscode=%{
    class SWIGTypeRegistryInitializer
    {
      public SWIGTypeRegistryInitializer()
      {
        SWIGInitializeTypeRegistry();
      }
    }
    private static SWIGTypeRegistryInitializer SWIGTypeRegistryInitializerInstance = new SWIGTypeRegistryInitializer();
%}
