
%ignore Urho3D::RefCounted::SetDeleter;
%ignore Urho3D::RefCounted::HasDeleter;

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
          var type = $imclassname.SWIGTypeRegistry[$imclassname.$csclazznameSWIGTypeId(cPtr)];
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

    %typemap(csbody_derived) TYPE %{
      private global::System.Runtime.InteropServices.HandleRef swigCPtr;
      private static InstanceCache<$csclassname> _instanceCache = new InstanceCache<$csclassname>();
      internal new static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn)
      {
        if (cPtr == global::System.IntPtr.Zero)
          return null;
        return _instanceCache.GetOrAdd(cPtr, () => {
          var type = $imclassname.SWIGTypeRegistry[$imclassname.$csclazznameSWIGTypeId(cPtr)];
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
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
        _instanceCache.AddNew(swigCPtr);
      }

      internal static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
        return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
      }
    %}

    // SharedPtr
    %refobject   TYPE "$this->AddRef();"  // Added in typemap above on object construction
    %unrefobject TYPE "$this->ReleaseRef();"

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
%enddef

%include "_refcounted.i"
//%interface_custom("%s", "I%s", Urho3D::RefCounted);
%include "Urho3D/Container/RefCounted.h"
%include "Urho3D/Container/Ptr.h"
