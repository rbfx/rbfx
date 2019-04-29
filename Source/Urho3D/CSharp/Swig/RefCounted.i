// This could be optimized by modifying shared_ptr<> typemaps to store a raw pointer instead of new shared_ptr instance
// and increment/decrement refcount manually.
%define URHO3D_REFCOUNTED(TYPE)
    %typemap(csbody) TYPE %{
      private global::System.Runtime.InteropServices.HandleRef swigCPtr;
      private static InstanceCache<$csclassname> _instanceCache = new InstanceCache<$csclassname>();
      protected bool swigCMemOwn;
      internal static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn)
      {
        if (cPtr == global::System.IntPtr.Zero)
          return null;
        return _instanceCache.GetOrAdd(cPtr, () => {
          global::System.Type type;
          if (!$imclassname.SWIGTypeRegistry.TryGetValue($imclassname.$csclazznameSWIGTypeId(cPtr), out type))
            type = typeof($csclassname);
          $csclassname result = null;
          if (type == typeof($csclassname))
            result = new $csclassname(cPtr, cMemoryOwn);
          else
            result = ($csclassname)global::System.Activator.CreateInstance(type, global::System.Reflection.BindingFlags.Instance|global::System.Reflection.BindingFlags.NonPublic|global::System.Reflection.BindingFlags.Public, null, new object[]{cPtr, cMemoryOwn}, null);
          result.AddRef();
          return result;
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

    %typemap(csbody_derived, directorsetup="\n    SetupSwigDirector();") TYPE %{
      private global::System.Runtime.InteropServices.HandleRef swigCPtr;
      private static InstanceCache<$csclassname> _instanceCache = new InstanceCache<$csclassname>();
      internal new static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn)
      {
        if (cPtr == global::System.IntPtr.Zero)
          return null;
        return _instanceCache.GetOrAdd(cPtr, () => {
          global::System.Type type;
          if (!$imclassname.SWIGTypeRegistry.TryGetValue($imclassname.$csclazznameSWIGTypeId(cPtr), out type))
            type = typeof($csclassname);
          $csclassname result = null;
          if (type == typeof($csclassname))
            result = new $csclassname(cPtr, cMemoryOwn);
          else
            result = ($csclassname)global::System.Activator.CreateInstance(type, global::System.Reflection.BindingFlags.Instance|global::System.Reflection.BindingFlags.NonPublic|global::System.Reflection.BindingFlags.Public, null, new object[]{cPtr, cMemoryOwn}, null);
          result.AddRef();
          return result;
        });
      }

      internal $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn) : base($imclassname.$csclazznameSWIGUpcast(cPtr), cMemoryOwn) {
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);$directorsetup
        _instanceCache.AddNew(swigCPtr);
      }

      internal static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
        return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
      }
    %}

    // SharedPtr
    %refobject   TYPE "$this->AddRef();"  // Added in typemap above on object construction
    %unrefobject TYPE "$this->ReleaseRef();"

    %typemap(ctype)  eastl::shared_ptr<TYPE> "TYPE*"                               // c layer type
    %typemap(imtype) eastl::shared_ptr<TYPE> "global::System.IntPtr"               // pinvoke type
    %typemap(cstype) eastl::shared_ptr<TYPE> "$typemap(cstype, TYPE*)"             // c# type
    %typemap(in)     eastl::shared_ptr<TYPE> %{ $1 = eastl::shared_ptr<TYPE>($input); %}    // c to cpp
    %typemap(out)    eastl::shared_ptr<TYPE> %{ $result = $1.detach(); %}          // cpp to c
    %typemap(out)    TYPE*          %{ $result = $1;                   %}          // cpp to c
    %typemap(csin)   eastl::shared_ptr<TYPE> "$typemap(cstype, TYPE*).getCPtr($csinput).Handle"      // convert C# to pinvoke
    %typemap(csout, excode=SWIGEXCODE) eastl::shared_ptr<TYPE> {                   // convert pinvoke to C#
        var ret = $typemap(cstype, TYPE*).wrap($imcall, true);$excode
        return ret;
    }
    %typemap(directorin)  eastl::shared_ptr<TYPE> "$iminput"
    %typemap(directorout) eastl::shared_ptr<TYPE> "$cscall"

    %typemap(csvarin, excode=SWIGEXCODE2) eastl::shared_ptr<TYPE> & %{
    set {
        $imcall;$excode
    }
    %}
    %typemap(csvarout, excode=SWIGEXCODE2) eastl::shared_ptr<TYPE> & %{
    get {
        var ret = $typemap(cstype, TYPE*).wrap($imcall, true);$excode
        return ret;
    }
    %}

    %apply eastl::shared_ptr<TYPE> { eastl::shared_ptr<TYPE>& }

    %typemap(in) eastl::shared_ptr<TYPE> & %{
        $*1_ltype $1Ref($input);
        $1 = &$1Ref;
    %}
    %typemap(out) eastl::shared_ptr<TYPE> & %{ $result = $1->detach(); %}          // cpp to c

    // WeakPtr
    %apply eastl::shared_ptr<TYPE>  { eastl::weak_ptr<TYPE>  }
    %apply eastl::shared_ptr<TYPE>& { eastl::weak_ptr<TYPE>& }
    %typemap(out) eastl::weak_ptr<TYPE> %{ $result = $1.get();          %}         // cpp to c
    %typemap(out) eastl::weak_ptr<TYPE> & %{ $result = $1->get(); %}               // cpp to c
    %typemap(in) eastl::weak_ptr<TYPE> & %{
        $*1_ltype $1Ref($input);
        $1 = &$1Ref;
    %}

    %typemap(csfinalize) TYPE %{
      ~$csclassname() {
        lock (this) {
          if (swigCMemOwn) {
            if (Urho3DNet.Context.Instance.GetSubsystem<Urho3DNet.Script>().ReleaseRefOnMainThread(this as RefCounted)) {
              swigCMemOwn = false;
            }
          }
          Dispose();
        }
      }
    %}%enddef

%include "_refcounted.i"

//%interface_custom("%s", "I%s", Urho3D::RefCounted);
%include "Urho3D/Container/RefCounted.h"
