// Blittable struct for marshalling SharedPtr<InterfaceType, RefCounted> across P/Invoke boundary.
// Stores the interface pointer and the separate RefCounted pointer that manages its lifetime.
%pragma(csharp) moduleimports=%{
[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
public struct PtrPair
{
    public System.IntPtr ptr;
    public System.IntPtr refCounted;
}
%}

// This could be optimized by modifying shared_ptr<> typemaps to store a raw pointer instead of new shared_ptr instance
// and increment/decrement refcount manually.
%define URHO3D_REFCOUNTED(TYPE)
    %typemap(csbody) TYPE %{
      [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
      private global::System.Runtime.InteropServices.HandleRef swigCPtr;
      [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
      private bool _swigCMemOwn = false;
      [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
      private static System.Object _lock = new System.Object();
      public static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn) {
        // This function gets called when we want to turn a native cPtr into managed instance. For example when some
        // method returns a pointer to a wrapped object.
        if (cPtr == global::System.IntPtr.Zero)
          return null;
        lock (_lock)
        {
          // Obtain GCHandle stored in RefCounted.
          var handle = $imclassname.RefCounted_GetScriptObject(new global::System.Runtime.InteropServices.HandleRef(null, cPtr));
          $csclassname result = null;
          if (handle != global::System.IntPtr.Zero) {
            // Fetch managed object from returned GCHandle and return it as this particular cPtr already has a wrapper object instance.
            var gchandle = global::System.Runtime.InteropServices.GCHandle.FromIntPtr(handle);
            return ($csclassname)gchandle.Target;
          }
          // GCHandle is null. cPtr does not have a managed wrapper object yet. We will create one.
          global::System.Type type;
          // Most downstream class always owns instance of RefCounted.
          cMemoryOwn = true;
          // Look up a most downstream type in type registry. This is important, because it allows us to use correct
          // wrapper type for polymorphic classes. For example API may return "UIElement" type, but cPtr would actually be
          // of type "Text". Here we would fetch type "Text" from type registry.
          if (!$imclassname.SWIGTypeRegistry.TryGetValue($imclassname.$csclazznameSWIGTypeId(cPtr), out type))
            type = typeof($csclassname);
          if (type == typeof($csclassname))
            // A fast path when type is not polymorphic. Using activator is not exactly cheap.
            result = new $csclassname(cPtr, cMemoryOwn);
          else
            // Type is polymorphic. Use activator to construct object from opaque Type object.
            result = ($csclassname)global::System.Activator.CreateInstance(type, global::System.Reflection.BindingFlags.Instance|global::System.Reflection.BindingFlags.NonPublic|global::System.Reflection.BindingFlags.Public, null, new object[]{cPtr, cMemoryOwn}, null);
          return result;
        }
      }

      public $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn) {
        // Construct a wrapper object wrapping a native instance.
        // Only most downstream class gets cMemoryOwn=true. Base classes do not.
        _swigCMemOwn = cMemoryOwn;
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
        if (cMemoryOwn) {
          var isStrong = Refs() > 0;
          var handleType = isStrong ? global::System.Runtime.InteropServices.GCHandleType.Normal : global::System.Runtime.InteropServices.GCHandleType.Weak;
          SetScriptObject(global::System.Runtime.InteropServices.GCHandle.ToIntPtr(
            global::System.Runtime.InteropServices.GCHandle.Alloc(this, handleType)), isStrong);
        }
      }

      public static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
        return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
      }
    %}

    %typemap(csbody_derived, directorsetup="\n        SetupSwigDirector();") TYPE %{
      [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
      private global::System.Runtime.InteropServices.HandleRef swigCPtr;
      [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
      private bool _swigCMemOwn = false;
      [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
      private static System.Object _lock = new System.Object();
      public new static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn) {
        // This function gets called when we want to turn a native cPtr into managed instance. For example when some
        // method returns a pointer to a wrapped object.
        if (cPtr == global::System.IntPtr.Zero)
          return null;
        lock (_lock)
        {
          // Obtain GCHandle stored in RefCounted.
          var handle = $imclassname.RefCounted_GetScriptObject(new global::System.Runtime.InteropServices.HandleRef(null, cPtr));
          $csclassname result = null;
          if (handle != global::System.IntPtr.Zero) {
            // Fetch managed object from returned GCHandle and return it as this particular cPtr already has a wrapper object instance.
            var gchandle = global::System.Runtime.InteropServices.GCHandle.FromIntPtr(handle);
            return ($csclassname)gchandle.Target;
          }
          // GCHandle is null. cPtr does not have a managed wrapper object yet. We will create one.
          global::System.Type type;
          // Most downstream class always owns instance of RefCounted.
          cMemoryOwn = true;
          // Look up a most downstream type in type registry. This is important, because it allows us to use correct
          // wrapper type for polymorphic classes. For example API may return "UIElement" type, but cPtr would actually be
          // of type "Text". Here we would fetch type "Text" from type registry.
          if (!$imclassname.SWIGTypeRegistry.TryGetValue($imclassname.$csclazznameSWIGTypeId(cPtr), out type))
            type = typeof($csclassname);
          if (type == typeof($csclassname))
            // A fast path when type is not polymorphic. Using activator is not exactly cheap.
            result = new $csclassname(cPtr, cMemoryOwn);
          else
            // Type is polymorphic. Use activator to construct object from opaque Type object.
            result = ($csclassname)global::System.Activator.CreateInstance(type, global::System.Reflection.BindingFlags.Instance|global::System.Reflection.BindingFlags.NonPublic|global::System.Reflection.BindingFlags.Public, null, new object[]{cPtr, cMemoryOwn}, null);
          return result;
        }
      }

      public $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn) : base($imclassname.$csclazznameSWIGUpcast(cPtr), false) {
        // Construct a wrapper object wrapping a native instance.
        // Only most downstream class gets cMemoryOwn=true. Base classes do not.
        _swigCMemOwn = cMemoryOwn;
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);$directorsetup
        if (cMemoryOwn) {
          var isStrong = Refs() > 0;
          var handleType = isStrong ? global::System.Runtime.InteropServices.GCHandleType.Normal : global::System.Runtime.InteropServices.GCHandleType.Weak;
          SetScriptObject(global::System.Runtime.InteropServices.GCHandle.ToIntPtr(
            global::System.Runtime.InteropServices.GCHandle.Alloc(this, handleType)), isStrong);
        }
      }

      public static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
        return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
      }
    %}

    %typemap(csdispose) TYPE %{
      ~$csclassname() {
        if (!IsExpired) {
          Dispose(false);
        }
      }

      public void Dispose() {
        // Free object when refcounting is not in use. If object had any references added through it's lifetime - last
        // ReleaseRef() will invoke Dispose(true) instead and this call will be noop.
        if (!IsExpired && !IsScriptStrongRef())
          Dispose(true);
      }
    %}
    %typemap(csdispose_derived) TYPE ""

    %typemap(csdisposing, methodname="Dispose", methodmodifiers="protected", parameters="bool disposing") TYPE {
      lock(this) {
        $typemap(csdisposed_extra_early_optional, TYPE)
        // FreeGCHandle();
        if (_swigCMemOwn) {
          if (Refs() > 0)
            throw new global::System.InvalidOperationException($"Objects of type {GetType().Name} with active references can not be disposed.");
          _swigCMemOwn = false;
          if (!IsScriptStrongRef())
            $imcall;
        }

        if (swigCPtr.Handle != global::System.IntPtr.Zero)
          swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
        if (disposing) {
          global::System.GC.SuppressFinalize(this);
        }
        $typemap(csdisposed_extra_optional, TYPE)
      }
    }

    %typemap(csdisposing_derived, methodname="Dispose", methodmodifiers="protected", parameters="bool disposing") TYPE {
      lock(this) {
        $typemap(csdisposed_extra_early_optional, TYPE)
        if (_swigCMemOwn) {
          if (Refs() > 0)
            throw new global::System.InvalidOperationException($"Objects of type {GetType().Name} with active references can not be disposed.");
          _swigCMemOwn = false;
          if (!IsScriptStrongRef())
            $imcall;
        }
        if (swigCPtr.Handle != global::System.IntPtr.Zero)
          swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
        base.Dispose(disposing);
        $typemap(csdisposed_extra_optional, TYPE)
      }
    }

    // SharedPtr
    %typemap(ctype)  Urho3D::SharedPtr<TYPE> "TYPE*"                               // c layer type
    %typemap(imtype) Urho3D::SharedPtr<TYPE> "global::System.IntPtr"               // pinvoke type
    %typemap(cstype) Urho3D::SharedPtr<TYPE> "$typemap(cstype, TYPE*)"             // c# type
    %typemap(in)     Urho3D::SharedPtr<TYPE> %{ $1 = Urho3D::SharedPtr<TYPE>($input); %}    // c to cpp
    %typemap(out)    Urho3D::SharedPtr<TYPE> %{ $result = $1.Detach(); %}          // cpp to c
    %typemap(out)    TYPE*          %{ $result = $1;                   %}          // cpp to c
    %typemap(csin)   Urho3D::SharedPtr<TYPE> "global::$nspace.$typemap(cstype, TYPE*).getCPtr($csinput).Handle"      // convert C# to pinvoke
    %typemap(csout, excode=SWIGEXCODE) Urho3D::SharedPtr<TYPE> {                   // convert pinvoke to C#
        var ret = $typemap(cstype, TYPE*).wrap($imcall, true);$excode
        return ret;
    }
    %typemap(directorin)  Urho3D::SharedPtr<TYPE> "$iminput"
    %typemap(directorout) Urho3D::SharedPtr<TYPE> "$cscall"

    %typemap(csvarin, excode=SWIGEXCODE2) Urho3D::SharedPtr<TYPE> & %{
    set {
        $imcall;$excode
    }
    %}
    %typemap(csvarout, excode=SWIGEXCODE2) Urho3D::SharedPtr<TYPE> & %{
    get {
        var ret = $typemap(cstype, TYPE*).wrap($imcall, true);$excode
        return ret;
    }
    %}

    %apply Urho3D::SharedPtr<TYPE> { Urho3D::SharedPtr<TYPE>& }

    %typemap(in) Urho3D::SharedPtr<TYPE> & %{
        $*1_ltype $1Ref($input);
        $1 = &$1Ref;
    %}
    %typemap(out) Urho3D::SharedPtr<TYPE> & %{ $result = $1->Get(); %}             // cpp to c

    // WeakPtr
    %apply Urho3D::SharedPtr<TYPE>  { Urho3D::WeakPtr<TYPE>  }
    %apply Urho3D::SharedPtr<TYPE>& { Urho3D::WeakPtr<TYPE>& }
    %typemap(out) Urho3D::WeakPtr<TYPE> %{ $result = $1.Get();          %}         // cpp to c
    %typemap(out) Urho3D::WeakPtr<TYPE> & %{ $result = $1->Get(); %}               // cpp to c
    %typemap(in) Urho3D::WeakPtr<TYPE> & %{
        $*1_ltype $1Ref($input);
        $1 = &$1Ref;
    %}
%enddef

%define URHO3D_REFCOUNTED_INTERFACE(TYPE, BASE)
    // SharedPtr
    %typemap(ctype)  Urho3D::SharedPtr<TYPE, BASE> "BASE*"                               // c layer type
    %typemap(imtype) Urho3D::SharedPtr<TYPE, BASE> "global::System.IntPtr"               // pinvoke type
    %typemap(cstype) Urho3D::SharedPtr<TYPE, BASE> "$typemap(cstype, TYPE*)"             // c# type
    %typemap(in)     Urho3D::SharedPtr<TYPE, BASE> %{ $1 = Urho3D::SharedPtr<TYPE, BASE>(dynamic_cast<TYPE*>($input), $input); %}    // c to cpp
    %typemap(out)    Urho3D::SharedPtr<TYPE, BASE> %{ $result = $1.GetRefCounted(); $1.Detach(); %}          // cpp to c
    %typemap(out)    TYPE*                         %{ $result = $1;          %}          // cpp to c
    %typemap(csin)   Urho3D::SharedPtr<TYPE, BASE> "$csinput.GetInterfaceCPtr().Handle"      // convert C# to pinvoke
    %typemap(csout, excode=SWIGEXCODE) Urho3D::SharedPtr<TYPE, BASE> {                   // convert pinvoke to C#
        var ret = ($typemap(cstype, TYPE*))$typemap(cstype, BASE*).wrap($imcall, true);$excode
        return ret;
    }
    %typemap(directorin)  Urho3D::SharedPtr<TYPE, BASE> "$iminput"
    %typemap(directorout) Urho3D::SharedPtr<TYPE, BASE> "$cscall"

    %typemap(csvarin, excode=SWIGEXCODE2) Urho3D::SharedPtr<TYPE, BASE> & %{
    set {
        $imcall;$excode
    }
    %}
    %typemap(csvarout, excode=SWIGEXCODE2) Urho3D::SharedPtr<TYPE, BASE> & %{
    get {
        var ret = ($typemap(cstype, TYPE*))$typemap(cstype, BASE*).wrap($imcall, true);$excode
        return ret;
    }
    %}

    %apply Urho3D::SharedPtr<TYPE, BASE> { Urho3D::SharedPtr<TYPE, BASE>& }

    %typemap(in) Urho3D::SharedPtr<TYPE, BASE> & %{
        $*1_ltype $1Ref(dynamic_cast<TYPE*>($input), $input);
        $1 = &$1Ref;
    %}
    %typemap(out) Urho3D::SharedPtr<TYPE, BASE> & %{ $result = $1->GetRefCounted(); %}             // cpp to c
%enddef

%define URHO3D_REFCOUNTED_TWOPOINTER(TYPE, BASE)
    // csbody for TYPE.
    // Wrapper stores a PtrPair (two pointers): ptr = TYPE*, refCounted = BASE* (for lifetime).
    %typemap(csbody) TYPE %{
      [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
      internal PtrPair _pair;
      [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
      private bool _swigCMemOwn;

      private global::System.Runtime.InteropServices.HandleRef swigCPtr
      {
          get { return new global::System.Runtime.InteropServices.HandleRef(this, _pair.ptr); }
      }

      /// Wrap from a PtrPair (SharedPtr path — has both interface and refCounted pointers).
      internal static $csclassname wrap(PtrPair pair, bool cMemoryOwn) {
          if (pair.ptr == global::System.IntPtr.Zero)
              return null;
          if (pair.refCounted != global::System.IntPtr.Zero) {
              // Look up GCHandle stored on the native RefCounted object for identity preservation.
              var handle = $imclassname.RefCounted_GetScriptObject(
                  new global::System.Runtime.InteropServices.HandleRef(null, pair.refCounted));
              if (handle != global::System.IntPtr.Zero) {
                  var gchandle = global::System.Runtime.InteropServices.GCHandle.FromIntPtr(handle);
                  return ($csclassname)gchandle.Target;
              }
          }
          return new $csclassname(pair, cMemoryOwn);
      }

      /// Wrap from a raw pointer (only interface pointer — used for bare TYPE* returns).
      internal static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn) {
          return wrap(new PtrPair { ptr = cPtr, refCounted = global::System.IntPtr.Zero }, cMemoryOwn);
      }

      /// Constructor for SWIG-generated factory methods (raw pointer from C wrapper).
      public $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn) : this(new PtrPair { ptr = cPtr, refCounted = global::System.IntPtr.Zero }, cMemoryOwn) {}

      public $csclassname(PtrPair pair, bool cMemoryOwn) {
          _swigCMemOwn = cMemoryOwn;
          _pair = pair;
          if (cMemoryOwn && pair.refCounted != global::System.IntPtr.Zero) {
              // Store GCHandle on the native RefCounted object. Always strong because
              // SharedPtr implies strong ownership.
              $imclassname.RefCounted_SetScriptObject(
                  new global::System.Runtime.InteropServices.HandleRef(null, pair.refCounted),
                  global::System.Runtime.InteropServices.GCHandle.ToIntPtr(
                      global::System.Runtime.InteropServices.GCHandle.Alloc(this,
                          global::System.Runtime.InteropServices.GCHandleType.Normal)),
                  true);
          }
      }

      public static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
          return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
      }

      bool IsExpired => _pair.ptr == global::System.IntPtr.Zero;
    %}

    // csdisposing for TYPE. Release ownership via the RefCounted pointer (if available).
    %typemap(csdisposing, methodname="Dispose", methodmodifiers="protected", parameters="bool disposing") TYPE {
      lock(this) {
          if (_pair.ptr != global::System.IntPtr.Zero) {
              if (_swigCMemOwn && _pair.refCounted != global::System.IntPtr.Zero) {
                  _swigCMemOwn = false;
                  $imclassname.RefCounted_ReleaseRef(
                      new global::System.Runtime.InteropServices.HandleRef(null, _pair.refCounted));
              }
              _pair.ptr = global::System.IntPtr.Zero;
              _pair.refCounted = global::System.IntPtr.Zero;
          }
          if (disposing) {
              global::System.GC.SuppressFinalize(this);
          }
      }
    }

    %typemap(csdispose) TYPE %{
      ~$csclassname() {
        if (!IsExpired)
          Dispose(false);
      }
      public void Dispose() {
        if (!IsExpired)
          Dispose(true);
      }
    %}
    %typemap(csdispose_derived) TYPE ""

    // SharedPtr<TYPE, BASE> marshalling through PtrPair.
    // C layer: PtrPair* for params, PtrPair for return value.
    %typemap(ctype, out="PtrPair") SharedPtr<TYPE, BASE> "PtrPair*"
    %typemap(imtype, out="PtrPair") SharedPtr<TYPE, BASE> "ref PtrPair"
    %typemap(cstype) SharedPtr<TYPE, BASE> "$typemap(cstype, TYPE*)"

    // cpp → c: extract both pointers and detach (preserve refcount).
    %typemap(out) SharedPtr<TYPE, BASE> %{
        PtrPair result;
        result.ptr = $1.Get();
        result.refCounted = $1.GetRefCounted();
        $1.Detach();
        $result = result;
    %}

    // c → cpp: reconstruct SharedPtr from pointer pair (AddRef on refCounted).
    %typemap(in) SharedPtr<TYPE, BASE> %{
        $1 = Urho3D::SharedPtr<TYPE, BASE>(static_cast<TYPE*>($input->ptr), static_cast<Urho3D::RefCounted*>($input->refCounted));
    %}

    // C# → pinvoke: pass the PtrPair by reference.
    %typemap(csin) SharedPtr<TYPE, BASE> "ref $csinput._pair"

    // pinvoke → C#: wrap the returned PtrPair into a managed wrapper.
    %typemap(csout, excode=SWIGEXCODE) SharedPtr<TYPE, BASE> {
        var ret = ($typemap(cstype, TYPE*))$typemap(cstype, TYPE*).wrap($imcall, true);$excode
        return ret;
    }

    // SharedPtr<TYPE, BASE>&
    %apply SharedPtr<TYPE, BASE> { SharedPtr<TYPE, BASE>& }

    %typemap(in) SharedPtr<TYPE, BASE> & %{
        $*1_ltype $1Ref(static_cast<TYPE*>($input->ptr), static_cast<Urho3D::RefCounted*>($input->refCounted));
        $1 = &$1Ref;
    %}
    %typemap(out) SharedPtr<TYPE, BASE> & %{
        PtrPair result;
        result.ptr = $1->Get();
        result.refCounted = $1->GetRefCounted();
        $result = result;
    %}

    %typemap(csvarin, excode=SWIGEXCODE2) SharedPtr<TYPE, BASE> & %{
    set {
        $imcall;$excode
    }
    %}
    %typemap(csvarout, excode=SWIGEXCODE2) SharedPtr<TYPE, BASE> & %{
    get {
        var ret = ($typemap(cstype, TYPE*))$typemap(cstype, TYPE*).wrap($imcall, true);$excode
        return ret;
    }
    %}

    // WeakPtr<TYPE, BASE> — reuse SharedPtr typemaps.
    %apply SharedPtr<TYPE, BASE>  { WeakPtr<TYPE, BASE> }
    %apply SharedPtr<TYPE, BASE>& { WeakPtr<TYPE, BASE>& }
    %typemap(out) WeakPtr<TYPE, BASE> %{
        PtrPair result;
        result.ptr = $1.Get();
        result.refCounted = nullptr;  // WeakPtr stores RefCount*, not RefCounted*
        $result = result;
    %}
    %typemap(out) WeakPtr<TYPE, BASE> & %{
        PtrPair result;
        result.ptr = $1->Get();
        result.refCounted = nullptr;  // WeakPtr stores RefCount*, not RefCounted*
        $result = result;
    %}
    %typemap(in) WeakPtr<TYPE, BASE> & %{
        $*1_ltype $1Ref(static_cast<TYPE*>($input->ptr), static_cast<Urho3D::RefCounted*>($input->refCounted));
        $1 = &$1Ref;
    %}
%enddef

// TODO: Fix autoswig script to output these as well.
URHO3D_REFCOUNTED(Urho3D::RefCounted);
URHO3D_REFCOUNTED(Urho3D::NetworkObject);
URHO3D_REFCOUNTED(Urho3D::StaticNetworkObject);
URHO3D_REFCOUNTED(Urho3D::BehaviorNetworkObject);
%include "generated/Urho3D/_pre_refcounted.i";

//%interface_custom("%s", "I%s", Urho3D::RefCounted);
%director Urho3D::RefCounted;
%ignore Urho3D::RefCounted::RefCountPtr;
%ignore Urho3D::RefCount;
%csmethodmodifiers Urho3D::RefCounted::SetScriptObject "internal"
%csmethodmodifiers Urho3D::RefCounted::GetScriptObject "internal"
%include "Urho3D/Container/RefCounted.h"
%include "Urho3D/Container/Ptr.h"
