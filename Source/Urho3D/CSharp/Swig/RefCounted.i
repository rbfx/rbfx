
%ignore Urho3D::RefCounted::SetDeleter;
%ignore Urho3D::RefCounted::HasDeleter;
%ignore Urho3D::RefCounted::GetDeleterUserData;

%define URHO3D_SMART_PTR(PTR_TYPE, TYPE)
  %feature("smartptr", noblock=1) PTR_TYPE<TYPE>;
  //%refobject   TYPE "$this->AddRef();"
  %unrefobject TYPE "$this->ReleaseRef();"

  %typemap(ctype)  PTR_TYPE<TYPE> "TYPE*"                                // c layer type
  %typemap(imtype) PTR_TYPE<TYPE> "global::System.IntPtr"                // pinvoke type
  %typemap(cstype) PTR_TYPE<TYPE> "$typemap(cstype, TYPE*)"             // c# type
  %typemap(in)     PTR_TYPE<TYPE> %{ $1 = PTR_TYPE<TYPE>($input); %}     // c to cpp
  %typemap(out)    PTR_TYPE<TYPE> %{
                                     $result = $1.Get();
                                     $1.Get()->AddRef();
                                  %}                                     // cpp to c
  %typemap(out)    TYPE*          %{
                                     $result = $1;
                                     $1->AddRef();
                                  %}                                     // cpp to c
  %typemap(csin)   PTR_TYPE<TYPE> "$typemap(cstype, TYPE*).getCPtr($csinput).Handle"      // convert C# to pinvoke
  %typemap(csout, excode=SWIGEXCODE) PTR_TYPE<TYPE> {             // convert pinvoke to C#
      var ret = $typemap(cstype, TYPE*).wrap($imcall, true);$excode
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
      var ret = $typemap(cstype, TYPE*).wrap($imcall, true);$excode
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

%define URHO3D_REFCOUNTED(TYPE)
  URHO3D_SMART_PTR(Urho3D::SharedPtr, TYPE);
  URHO3D_SMART_PTR(Urho3D::WeakPtr,   TYPE);
%enddef

%include "_refcounted.i"
%include "Urho3D/Container/RefCounted.h"
%include "Urho3D/Container/Ptr.h"
