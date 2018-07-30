
%ignore Urho3D::RefCounted::SetDeleter;
%ignore Urho3D::RefCounted::HasDeleter;
%ignore Urho3D::RefCounted::GetDeleterUserData;

%define URHO3D_SMART_PTR(PTR_TYPE, TYPE, CSTYPE)
  %naturalvar PTR_TYPE<TYPE>;
  %feature("smartptr", noblock=1) PTR_TYPE<TYPE>;
  //%refobject   TYPE "$this->AddRef();"
  %unrefobject TYPE "$this->ReleaseRef();"

  %typemap(ctype)  PTR_TYPE<TYPE> "TYPE*"                                // c layer type
  %typemap(imtype) PTR_TYPE<TYPE> "global::System.IntPtr"                // pinvoke type
  %typemap(cstype) PTR_TYPE<TYPE> "CSTYPE"                               // c# type
  %typemap(in)     PTR_TYPE<TYPE> %{ $1 = PTR_TYPE<TYPE>($input); %}     // c to cpp
  %typemap(out)    PTR_TYPE<TYPE> %{
                                     $result = $1.Get();
                                     $1.Get()->AddRef();
                                  %}                                     // cpp to c
  %typemap(out)    TYPE*          %{
                                     $result = $1;
                                     $1->AddRef();
                                  %}                                     // cpp to c
  %typemap(csin)   PTR_TYPE<TYPE> "$typemap(cstype, PTR_TYPE<TYPE>).getCPtr($csinput).Handle"      // convert C# to pinvoke
  %typemap(csout, excode=SWIGEXCODE) PTR_TYPE<TYPE>, TYPE* {             // convert pinvoke to C#
      var ret = new $typemap(cstype, PTR_TYPE<TYPE>)($imcall, true);$excode
      return ret;
    }
  %typemap(directorin)  PTR_TYPE<TYPE> "$iminput"
  %typemap(directorout) PTR_TYPE<TYPE> "$cscall"

  %typemap(csvarin, excode=SWIGEXCODE2) PTR_TYPE<TYPE> & %{
    set {
      $imcall;$excode
    }
  %}
  %typemap(csvarout, excode=SWIGEXCODE2) PTR_TYPE<TYPE> & %{
    get {
      var ret = new $typemap(cstype, PTR_TYPE<TYPE>)($imcall, true);$excode
      return ret;
    }
  %}

  %apply PTR_TYPE<TYPE> { PTR_TYPE<TYPE>& }

  %typemap(in) PTR_TYPE<TYPE> & %{
    $*1_ltype $1_tmp($input);
    $1 = &$1_tmp;
    $1->Get()->AddRef();
  %}
  %typemap(out) PTR_TYPE<TYPE> & %{
    $result = $1->Get();
    $1->Get()->AddRef();
  %}               // cpp to c

%enddef

%define URHO3D_REFCOUNTED(NS, TYPE)
  URHO3D_SMART_PTR(Urho3D::SharedPtr, NS::TYPE, global::Urho3DNet.TYPE);
  URHO3D_SMART_PTR(Urho3D::WeakPtr, NS::TYPE, global::Urho3DNet.TYPE);

  // Base proxy classes
  %typemap(csbody) NS::TYPE %{
    private global::System.Runtime.InteropServices.HandleRef swigCPtr;
    private bool swigCMemOwnBase;

    internal $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn) {
      swigCMemOwnBase = cMemoryOwn;
      swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
    }

    internal static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
      return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
    }
  %}

  // Derived proxy classes
  %typemap(csbody_derived) NS::TYPE %{
    private global::System.Runtime.InteropServices.HandleRef swigCPtr;
    private bool swigCMemOwnDerived;

    internal $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn) : base($imclassname.$csclazznameSWIGUpcast(cPtr), false/*was true*/) {
      swigCMemOwnDerived = cMemoryOwn;
      swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
    }

    internal static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
      return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
    }
  %}

  %typemap(csdestruct, methodname="Dispose", methodmodifiers="public") NS::TYPE {
      lock(this) {
        if (swigCPtr.Handle != global::System.IntPtr.Zero) {
          if (swigCMemOwnBase) {
            swigCMemOwnBase = false;
            $imcall;
          }
          swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
        }
        global::System.GC.SuppressFinalize(this);
      }
    }

  %typemap(csdestruct_derived, methodname="Dispose", methodmodifiers="public") NS::TYPE {
      lock(this) {
        if (swigCPtr.Handle != global::System.IntPtr.Zero) {
          if (swigCMemOwnDerived) {
            swigCMemOwnDerived = false;
            $imcall;
          }
          swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
        }
        global::System.GC.SuppressFinalize(this);
        base.Dispose();
      }
    }
%enddef

%include "_refcounted.i"
%include "Urho3D/Container/RefCounted.h"
%include "Urho3D/Container/Ptr.h"
