// This could be optimized by modifying shared_ptr<> typemaps to store a raw pointer instead of new shared_ptr instance
// and increment/decrement refcount manually.
%define URHO3D_REFCOUNTED(TYPE)
    %typemap(csbody) TYPE %{
      private global::System.Runtime.InteropServices.HandleRef swigCPtr;
      private bool swigCMemOwn;
      internal static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn) {
        if (cPtr == global::System.IntPtr.Zero)
          return null;

        cMemoryOwn = true;  // Managed instances always keep reference to subclass of RefCounted
        var handle = $imclassname.RefCounted_GetScriptObject(new System.Runtime.InteropServices.HandleRef(null, cPtr));
        $csclassname result = null;
        if (handle != global::System.IntPtr.Zero) {
          var gchandle = global::System.Runtime.InteropServices.GCHandle.FromIntPtr(handle);
          result = ($csclassname)gchandle.Target;
          if (result == null) {
            // Happens when this is not a director class and reference to managed object was lost.
            gchandle.Free();
          } else if (!result.GetOwnsMemory()) {
            // Happens when managed code had lost all references to a director class object.
            result.AddRef();
            gchandle.Free();
            result.SetOwnsMemory(true);
            result.SetScriptObject(global::System.Runtime.InteropServices.GCHandle.ToIntPtr(global::System.Runtime.InteropServices.GCHandle.Alloc(result, global::System.Runtime.InteropServices.GCHandleType.WeakTrackResurrection)));
          }
        }

        if (result == null) {
          global::System.Type type;
          if (!$imclassname.SWIGTypeRegistry.TryGetValue($imclassname.$csclazznameSWIGTypeId(cPtr), out type))
            type = typeof($csclassname);
          if (type == typeof($csclassname))
            result = new $csclassname(cPtr, cMemoryOwn);
          else
            result = ($csclassname)global::System.Activator.CreateInstance(type, global::System.Reflection.BindingFlags.Instance|global::System.Reflection.BindingFlags.NonPublic|global::System.Reflection.BindingFlags.Public, null, new object[]{cPtr, cMemoryOwn}, null);
        }
        return result;
      }

      internal $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn) {
        swigCMemOwn = cMemoryOwn;
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
        if (cMemoryOwn) {
          SetScriptObject(global::System.Runtime.InteropServices.GCHandle.ToIntPtr(global::System.Runtime.InteropServices.GCHandle.Alloc(this, global::System.Runtime.InteropServices.GCHandleType.WeakTrackResurrection)));
          AddRef();    // This code path is invoked when user inherits from wrapped class, and only from top-most wrapper constructor.
        }
      }

      internal static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
        return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
      }

      internal virtual bool GetOwnsMemory() { return swigCMemOwn; }
      internal virtual void SetOwnsMemory(bool owns) { swigCMemOwn = owns; }
    %}

    %typemap(csbody_derived, directorsetup="\n        SetupSwigDirector();") TYPE %{
      private global::System.Runtime.InteropServices.HandleRef swigCPtr;
      private bool swigCMemOwn;
      internal static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn) {
        if (cPtr == global::System.IntPtr.Zero)
          return null;

        cMemoryOwn = true;  // Managed instances always keep reference to subclass of RefCounted
        var handle = $imclassname.RefCounted_GetScriptObject(new System.Runtime.InteropServices.HandleRef(null, cPtr));
        $csclassname result = null;
        if (handle != global::System.IntPtr.Zero) {
          var gchandle = global::System.Runtime.InteropServices.GCHandle.FromIntPtr(handle);
          result = ($csclassname)gchandle.Target;
          if (result == null) {
            // Happens when this is not a director class and reference to managed object was lost.
            gchandle.Free();
          } else if (!result.GetOwnsMemory()) {
            // Happens when managed code had lost all references to a director class object.
            gchandle.Free();
            result.SetOwnsMemory(true);
            result.SetScriptObject(global::System.Runtime.InteropServices.GCHandle.ToIntPtr(global::System.Runtime.InteropServices.GCHandle.Alloc(result, global::System.Runtime.InteropServices.GCHandleType.WeakTrackResurrection)));
          }
        }

        if (result == null) {
          global::System.Type type;
          if (!$imclassname.SWIGTypeRegistry.TryGetValue($imclassname.$csclazznameSWIGTypeId(cPtr), out type))
            type = typeof($csclassname);
          if (type == typeof($csclassname))
            result = new $csclassname(cPtr, cMemoryOwn);
          else
            result = ($csclassname)global::System.Activator.CreateInstance(type, global::System.Reflection.BindingFlags.Instance|global::System.Reflection.BindingFlags.NonPublic|global::System.Reflection.BindingFlags.Public, null, new object[]{cPtr, cMemoryOwn}, null);
        }
        return result;
      }

      internal $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn) : base($imclassname.$csclazznameSWIGUpcast(cPtr), false) {
        swigCMemOwn = cMemoryOwn;
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);$directorsetup
        if (cMemoryOwn) {
          SetScriptObject(global::System.Runtime.InteropServices.GCHandle.ToIntPtr(global::System.Runtime.InteropServices.GCHandle.Alloc(this, global::System.Runtime.InteropServices.GCHandleType.WeakTrackResurrection)));
          AddRef();    // This code path is invoked when user inherits from wrapped class, and only from top-most wrapper constructor.
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
    %typemap(out) Urho3D::SharedPtr<TYPE> & %{ $result = $1->Detach(); %}          // cpp to c

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


  /*
  Disposing from finalizers is special when it comes to directors. Native class depends on managed instance for logic
  implementation. When we loose all managed references and references are held on native side (so Refs() > 1) we do
  following:
  1. Replace weak gchandle with strong one to prevent deallocation.
  2. ReleaseRef() - this managed object is OK with being deleted as soon as native code no longer holds any references.
  3. Set swigCMemOwn=false to reflect released reference.

  When Refs() == 1 then we proceed with object deallocation as usual. This was last reference anyway.

  It is possible that managed code will reacquire a reference to this managed object later on. This leads to a situation
  where class is alive but does not hold a reference. This case is handled in RefCounted.i - managed instance is reused,
  we add a reference and set swigCMemOwn=true.
  */

  %typemap(csdisposing, methodname="Dispose", methodmodifiers="protected", parameters="bool disposing") TYPE {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          var handle = GetScriptObject();
          if (handle != global::System.IntPtr.Zero) {
            global::System.Runtime.InteropServices.GCHandle.FromIntPtr(handle).Free();
            SetScriptObject(global::System.IntPtr.Zero);
          }
          var numReferences = Refs();
          if (numReferences <= 0)
            throw new global::System.Exception("Wrapper owns instance which expired already. Wrapper is broken.");
          if (disposing) {
            if (numReferences > 0 && typeof($csclassname) != GetType())
              throw new global::System.Exception("Disposing of this class is not allowed because native code still holds a reference and on managed instance for behavior implementation.");
            ReleaseRef();
          } else {
            if (numReferences > 1) {
              SetScriptObject(global::System.Runtime.InteropServices.GCHandle.ToIntPtr(global::System.Runtime.InteropServices.GCHandle.Alloc(this)));
              global::System.GC.ReRegisterForFinalize(this);
              ReleaseRef();
            } else {
              var script = Urho3DNet.Context.Instance?.GetSubsystem<Urho3DNet.Script>();
              if (script != null)
                script.ReleaseRefOnMainThread(this);
              else
                ReleaseRef();
            }
          }
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
    }
  }

  %typemap(csdisposing_derived, methodname="Dispose", methodmodifiers="protected", parameters="bool disposing") TYPE {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          var handle = GetScriptObject();
          if (handle != global::System.IntPtr.Zero) {
            global::System.Runtime.InteropServices.GCHandle.FromIntPtr(handle).Free();
            SetScriptObject(global::System.IntPtr.Zero);
          }
          var numReferences = Refs();
          if (numReferences <= 0)
            throw new global::System.Exception("Wrapper owns instance which expired already. Wrapper is broken.");
          if (disposing) {
            if (numReferences > 0 && typeof($csclassname) != GetType())
              throw new global::System.Exception("Disposing of this class is not allowed because native code still holds a reference and on managed instance for behavior implementation.");
            ReleaseRef();
          } else {
            if (numReferences > 1) {
              SetScriptObject(global::System.Runtime.InteropServices.GCHandle.ToIntPtr(global::System.Runtime.InteropServices.GCHandle.Alloc(this)));
              global::System.GC.ReRegisterForFinalize(this);
              ReleaseRef();
            } else {
              var script = Urho3DNet.Context.Instance?.GetSubsystem<Urho3DNet.Script>();
              if (script != null)
                script.ReleaseRefOnMainThread(this);
              else
                ReleaseRef();
            }
          }
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      base.Dispose(disposing);
    }
  }
%enddef

%include "_refcounted.i"
//%interface_custom("%s", "I%s", Urho3D::RefCounted);
%ignore Urho3D::RefCounted::RefCountPtr;
%csmethodmodifiers Urho3D::RefCounted::SetScriptObject "internal"
%csmethodmodifiers Urho3D::RefCounted::GetScriptObject "internal"
%include "Urho3D/Container/RefCounted.h"
%include "Urho3D/Container/Ptr.h"
