// This could be optimized by modifying shared_ptr<> typemaps to store a raw pointer instead of new shared_ptr instance
// and increment/decrement refcount manually.
%define URHO3D_REFCOUNTED(TYPE)
    %typemap(csbody) TYPE %{
      [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
      private global::System.Runtime.InteropServices.HandleRef swigCPtr;
      [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
      private bool swigCMemOwn;
      internal static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn) {
        if (cPtr == global::System.IntPtr.Zero)
          return null;

        cMemoryOwn = true;  // Managed instances always keep reference to subclass of RefCounted
        var handle = $imclassname.RefCounted_GetScriptObject(new global::System.Runtime.InteropServices.HandleRef(null, cPtr));
        $csclassname result = null;
        if (handle != global::System.IntPtr.Zero) {
          var gchandle = global::System.Runtime.InteropServices.GCHandle.FromIntPtr(handle);
          return ($csclassname)gchandle.Target;
        }

        global::System.Type type;
        if (!$imclassname.SWIGTypeRegistry.TryGetValue($imclassname.$csclazznameSWIGTypeId(cPtr), out type))
          type = typeof($csclassname);
        if (type == typeof($csclassname))
          result = new $csclassname(cPtr, cMemoryOwn);
        else
          result = ($csclassname)global::System.Activator.CreateInstance(type, global::System.Reflection.BindingFlags.Instance|global::System.Reflection.BindingFlags.NonPublic|global::System.Reflection.BindingFlags.Public, null, new object[]{cPtr, cMemoryOwn}, null);
        return result;
      }

      internal $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn) {
        swigCMemOwn = cMemoryOwn;
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
        if (cMemoryOwn) {
          bool strongRef = AddRef() > 1;
          SetScriptObject(global::System.Runtime.InteropServices.GCHandle.ToIntPtr(
            global::System.Runtime.InteropServices.GCHandle.Alloc(this,
              strongRef ? global::System.Runtime.InteropServices.GCHandleType.Normal :
                          global::System.Runtime.InteropServices.GCHandleType.Weak)));
        }
      }

      internal static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
        return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
      }

      internal virtual bool GetOwnsMemory() { return swigCMemOwn; }
      internal virtual void SetOwnsMemory(bool owns) { swigCMemOwn = owns; }
    %}

    %typemap(csbody_derived, directorsetup="\n        SetupSwigDirector();") TYPE %{
      [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
      private global::System.Runtime.InteropServices.HandleRef swigCPtr;
      [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
      private bool swigCMemOwn;
      internal static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn) {
        if (cPtr == global::System.IntPtr.Zero)
          return null;

        cMemoryOwn = true;  // Managed instances always keep reference to subclass of RefCounted
        var handle = $imclassname.RefCounted_GetScriptObject(new global::System.Runtime.InteropServices.HandleRef(null, cPtr));
        $csclassname result = null;
        if (handle != global::System.IntPtr.Zero) {
          var gchandle = global::System.Runtime.InteropServices.GCHandle.FromIntPtr(handle);
          return ($csclassname)gchandle.Target;
        }

        global::System.Type type;
        if (!$imclassname.SWIGTypeRegistry.TryGetValue($imclassname.$csclazznameSWIGTypeId(cPtr), out type))
          type = typeof($csclassname);
        if (type == typeof($csclassname))
          result = new $csclassname(cPtr, cMemoryOwn);
        else
          result = ($csclassname)global::System.Activator.CreateInstance(type, global::System.Reflection.BindingFlags.Instance|global::System.Reflection.BindingFlags.NonPublic|global::System.Reflection.BindingFlags.Public, null, new object[]{cPtr, cMemoryOwn}, null);
        return result;
      }

      internal $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn) : base($imclassname.$csclazznameSWIGUpcast(cPtr), false) {
        swigCMemOwn = cMemoryOwn;
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);$directorsetup
        if (cMemoryOwn) {
          bool strongRef = AddRef() > 1;
          SetScriptObject(global::System.Runtime.InteropServices.GCHandle.ToIntPtr(
            global::System.Runtime.InteropServices.GCHandle.Alloc(this,
              strongRef ? global::System.Runtime.InteropServices.GCHandleType.Normal :
                          global::System.Runtime.InteropServices.GCHandleType.Weak)));
        }
      }

      internal static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
        return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
      }

      internal override bool GetOwnsMemory() { return swigCMemOwn; }
      internal override void SetOwnsMemory(bool owns) { swigCMemOwn = owns; }
    %}

    // C# wrapper does this instead
    %refobject   TYPE "//$this->AddRef();        // Called by C# wrapper wrap() or constructor."
    %unrefobject TYPE "//$this->ReleaseRef();    // Called by C# wrapper Dispose().\n  assert(false);   // This destructor should never be called, ReleaseRef() handles destruction."

    // SharedPtr
    %typemap(ctype)  Urho3D::SharedPtr<TYPE> "TYPE*"                               // c layer type
    %typemap(imtype) Urho3D::SharedPtr<TYPE> "global::System.IntPtr"               // pinvoke type
    %typemap(cstype) Urho3D::SharedPtr<TYPE> "$typemap(cstype, TYPE*)"             // c# type
    %typemap(in)     Urho3D::SharedPtr<TYPE> %{ $1 = Urho3D::SharedPtr<TYPE>($input); %}    // c to cpp
    %typemap(out)    Urho3D::SharedPtr<TYPE> %{ $result = $1.Detach(); %}          // cpp to c
    %typemap(out)    TYPE*          %{ $result = $1;                   %}          // cpp to c
    %typemap(csin)   Urho3D::SharedPtr<TYPE> "$typemap(cstype, TYPE*).getCPtr($csinput).Handle"      // convert C# to pinvoke
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

    %typemap(csdispose) TYPE %{
      ~$csclassname() {
        lock (this) {
          Dispose(false);
        }
      }

      public void Dispose() {
        Dispose(true);
        global::System.GC.SuppressFinalize(this);
      }
    %}
    %typemap(csdispose_derived) TYPE ""

  %typemap(csdisposing, methodname="Dispose", methodmodifiers="protected", parameters="bool disposing") TYPE {
    lock(this) {
      $typemap(csdisposed_extra_early_optional, TYPE)
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          var numReferences = Refs();
          if (numReferences <= 0)
            throw new global::System.InvalidOperationException("Wrapper owns instance which expired already. Wrapper is broken.");

          var handle = SwapScriptObject(global::System.IntPtr.Zero);
          if (disposing) {
            if (numReferences > 1)
              throw new global::System.InvalidOperationException($"Disposing of '{GetType().Name}' object that is still referenced is not allowed.");
            ReleaseRef();           // Possible native object deallocation
          } else {
            if (numReferences > 1 && GetType() != typeof($csclassname))
              throw new global::System.InvalidOperationException($"Disposing of '{GetType().Name}' object that is still referenced is not allowed. Wrapper is broken.");
            ReleaseRefSafe();       // Possible native object deallocation on the main thread on next frame
          }
          if (handle != global::System.IntPtr.Zero) {
            global::System.Runtime.InteropServices.GCHandle.FromIntPtr(handle).Free();
          }
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      $typemap(csdisposed_extra_optional, TYPE)
    }
  }

  %typemap(csdisposing_derived, methodname="Dispose", methodmodifiers="protected", parameters="bool disposing") TYPE {
    lock(this) {
      $typemap(csdisposed_extra_early_optional, TYPE)
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        $typemap(csdisposing_extra_optional, TYPE)
        if (swigCMemOwn) {
          swigCMemOwn = false;
          var numReferences = Refs();
          if (numReferences <= 0)
            throw new global::System.Exception("Wrapper owns instance which expired already. Wrapper is broken.");

          if (numReferences > 1 && GetType() != typeof($csclassname))
            throw new global::System.Exception("Disposing of object that is still referenced is not allowed.");
          var handle = SwapScriptObject(global::System.IntPtr.Zero);
          if (disposing) {
            if (numReferences > 1)
              throw new global::System.InvalidOperationException($"Disposing of '{GetType().Name}' object that is still referenced is not allowed.");
            ReleaseRef();           // Possible native object deallocation
          } else {
            if (numReferences > 1 && GetType() != typeof($csclassname))
              throw new global::System.InvalidOperationException($"Disposing of '{GetType().Name}' object that is still referenced is not allowed. Wrapper is broken.");
            ReleaseRefSafe();       // Possible native object deallocation on the main thread on next frame
          }
          if (handle != global::System.IntPtr.Zero) {
            global::System.Runtime.InteropServices.GCHandle.FromIntPtr(handle).Free();
          }
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      base.Dispose(disposing);
      $typemap(csdisposed_extra_optional, TYPE)
    }
  }
%enddef

// TODO: Fix autoswig script to output these as well.
URHO3D_REFCOUNTED(Urho3D::RefCounted);
URHO3D_REFCOUNTED(Urho3D::ShaderProgram);

%include "_refcounted.i"
//%interface_custom("%s", "I%s", Urho3D::RefCounted);
%director Urho3D::RefCounted;
%ignore Urho3D::RefCounted::RefCountPtr;
%ignore Urho3D::RefCount;
%csmethodmodifiers Urho3D::RefCounted::SetScriptObject "internal"
%csmethodmodifiers Urho3D::RefCounted::GetScriptObject "internal"
%csmethodmodifiers Urho3D::RefCounted::SwapScriptObject "internal"
%include "Urho3D/Container/RefCounted.h"
%include "Urho3D/Container/Ptr.h"
