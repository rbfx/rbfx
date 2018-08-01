
// Proxy classes (base classes, ie, not derived classes)
%typemap(csbody) SWIGTYPE %{
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  private static InstanceCache<$csclassname> _instanceCache = new InstanceCache<$csclassname>();
  protected bool swigCMemOwn;
  internal static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn)
  {
    return _instanceCache.GetOrAdd(cPtr, () => {
      var type = $imclassname.SWIGTypeRegistry[$imclassname.$csclassname_SWIGTypeId(cPtr)];
      if (type == typeof($csclassname))
        return new $csclassname(cPtr, cMemoryOwn, false);
      return ($csclassname)global::System.Activator.CreateInstance(type, global::System.Reflection.BindingFlags.Instance|global::System.Reflection.BindingFlags.NonPublic|global::System.Reflection.BindingFlags.Public, null, new object[]{cPtr, cMemoryOwn, false}, null);
    });
  }

  internal $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn, bool cache=true) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
    if (cache)
      _instanceCache.Add(swigCPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }
%}

// Derived proxy classes
%typemap(csbody_derived) SWIGTYPE %{
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  private static InstanceCache<$csclassname> _instanceCache = new InstanceCache<$csclassname>();
  internal static new $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn)
  {
    return _instanceCache.GetOrAdd(cPtr, () => {
      var type = $imclassname.SWIGTypeRegistry[$imclassname.$csclassname_SWIGTypeId(cPtr)];
      if (type == typeof($csclassname))
        return new $csclassname(cPtr, cMemoryOwn, false);
      return ($csclassname)global::System.Activator.CreateInstance(type, global::System.Reflection.BindingFlags.Instance|global::System.Reflection.BindingFlags.NonPublic|global::System.Reflection.BindingFlags.Public, null, new object[]{cPtr, cMemoryOwn, false}, null);
    });
  }

  internal $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn, bool cache=true) : base($imclassname.$csclazznameSWIGUpcast(cPtr), cMemoryOwn) {
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
    if (cache)
      _instanceCache.Add(swigCPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }
%}

%typemap(csdestruct, methodname="Dispose", methodmodifiers="public") SWIGTYPE {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        _instanceCache.Remove(swigCPtr);
        if (swigCMemOwn) {
          swigCMemOwn = false;
          $imcall;
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

%typemap(csdestruct_derived, methodname="Dispose", methodmodifiers="public") SWIGTYPE {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          $imcall;
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
      base.Dispose();
    }
  }

%typemap(csdirectorin) SWIGTYPE "$&csclassname.wrap($iminput, true)"
%typemap(csdirectorin) SWIGTYPE *, SWIGTYPE (CLASS::*) "($iminput == global::System.IntPtr.Zero) ? null : $csclassname.wrap($iminput, false)"
%typemap(csdirectorin) SWIGTYPE & "$csclassname.wrap($iminput, false)"
%typemap(csdirectorin) SWIGTYPE && "$csclassname.wrap($iminput, false)"

%typemap(csout, excode=SWIGEXCODE) SWIGTYPE {
    $&csclassname ret = $&csclassname.wrap($imcall, true);$excode
    return ret;
  }
%typemap(csout, excode=SWIGEXCODE) SWIGTYPE & {
    $csclassname ret = $csclassname.wrap($imcall, $owner);$excode
    return ret;
  }
%typemap(csout, excode=SWIGEXCODE) SWIGTYPE && {
    $csclassname ret = $csclassname.wrap($imcall, $owner);$excode
    return ret;
  }
%typemap(csout, excode=SWIGEXCODE) SWIGTYPE *, SWIGTYPE [] {
    global::System.IntPtr cPtr = $imcall;
    $csclassname ret = (cPtr == global::System.IntPtr.Zero) ? null : $csclassname.wrap(cPtr, $owner);$excode
    return ret;
  }
%typemap(csout, excode=SWIGEXCODE) SWIGTYPE (CLASS::*) {
    string cMemberPtr = $imcall;
    $csclassname ret = (cMemberPtr == null) ? null : $csclassname.wrap(cMemberPtr, $owner);$excode
    return ret;
  }
%typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE %{
    get {
      $&csclassname ret = $&csclassname.wrap($imcall, true);$excode
      return ret;
    } %}
%typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE & %{
    get {
      $csclassname ret = $csclassname.wrap($imcall, $owner);$excode
      return ret;
    } %}
%typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE && %{
    get {
      $csclassname ret = $csclassname.wrap($imcall, $owner);$excode
      return ret;
    } %}
%typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE *, SWIGTYPE [] %{
    get {
      global::System.IntPtr cPtr = $imcall;
      $csclassname ret = (cPtr == global::System.IntPtr.Zero) ? null : $csclassname.wrap(cPtr, $owner);$excode
      return ret;
    } %}

%typemap(csvarout, excode=SWIGEXCODE2) SWIGTYPE (CLASS::*) %{
    get {
      string cMemberPtr = $imcall;
      $csclassname ret = (cMemberPtr == null) ? null : $csclassname.wrap(cMemberPtr, $owner);$excode
      return ret;
    } %}
%typemap(csout, excode=SWIGEXCODE) SWIGTYPE *const& {
    global::System.IntPtr cPtr = $imcall;
    $*csclassname ret = (cPtr == global::System.IntPtr.Zero) ? null : $*csclassname.wrap(cPtr, $owner);$excode
    return ret;
  }

%typemap(csbody) SWIGTYPE *, SWIGTYPE &, SWIGTYPE &&, SWIGTYPE [] %{
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  private static InstanceCache<$csclassname> _instanceCache = new InstanceCache<$csclassname>();
  internal static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn)
  {
    return _instanceCache.GetOrAdd(cPtr, () => new $csclassname(cPtr, cMemoryOwn, false));
  }

  internal $csclassname(global::System.IntPtr cPtr, bool futureUse, bool cache=true) {
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
    if (cache)
      _instanceCache.Add(swigCPtr);
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
