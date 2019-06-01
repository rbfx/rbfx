%include "Helpers.i"

// Types that are binary-compatible
%define URHO3D_BINARY_COMPATIBLE_TYPE_EX(CS_TYPE, CPP_TYPE, POD_TYPE)
    %ignore CPP_TYPE;
    struct CPP_TYPE;

    %typemap(ctype, out="POD_TYPE") CPP_TYPE, const CPP_TYPE &, CPP_TYPE & "CPP_TYPE *"
    %typemap(imtype, out="CS_TYPE")
                                   CPP_TYPE, const CPP_TYPE &, CPP_TYPE & "ref CS_TYPE"
    %typemap(cstype)               CPP_TYPE, const CPP_TYPE &             "CS_TYPE"
    %typemap(cstype, out="CS_TYPE")                            CPP_TYPE & "ref CS_TYPE"

    // Parameters
    %typemap(in, canthrow=1) CPP_TYPE %{
        if (!$input) {
            SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null", 0);
            return $null;
        }
        $1 = *$input;
    %}
    %typemap(in, canthrow=1) CPP_TYPE & %{
        if (!$input) {
            SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null", 0);
            return $null;
        }
        $*1_ltype $inputRef(*$input);
        $1 = &$inputRef;
    %}
    %typemap(argout)       CPP_TYPE & %{ *$input = $inputRef; %}
    %typemap(argout)                 const CPP_TYPE & ""
    %typemap(csin)         CPP_TYPE, const CPP_TYPE &, CPP_TYPE & "ref $csinput"
    %typemap(directorin)   CPP_TYPE & "$input = addr($1);"
    %typemap(directorin)   const CPP_TYPE &, CPP_TYPE %{ $input = addr($1); %}
    %typemap(csdirectorin) const CPP_TYPE &, CPP_TYPE "$iminput"
    %typemap(csdirectorin) CPP_TYPE &                 "ref $iminput"


    // Returns
    %typemap(out, null="POD_TYPE{}")   const CPP_TYPE &, CPP_TYPE &, CPP_TYPE %{ $result = pod::convert<CPP_TYPE, POD_TYPE>(deref($1)); %}
    %typemap(csout, excode=SWIGEXCODE) const CPP_TYPE &, CPP_TYPE &, CPP_TYPE  {
        unsafe {
            var res = $imcall;$excode
            return res;
        }
    }
    %typemap(csdirectorout)           CPP_TYPE, const CPP_TYPE &, CPP_TYPE & "$cscall"
    %typemap(directorout, canthrow=1) CPP_TYPE %{
        if (!$input) {
            SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null", 0);
            return $null;
        }
        $result = *$input;
    %}
    %typemap(directorout, canthrow=1) const CPP_TYPE &, CPP_TYPE & %{
        if (!$input) {
            SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null", 0);
            return $null;
        }
        static thread_local CPP_TYPE $inputStatic(*$input);
        $result = &$inputStatic;
    %}

    // Variables
    %typemap(csvarin, excode=SWIGEXCODE2) CPP_TYPE &, CPP_TYPE * %{
        set {
            $imcall;$excode
        }
    %}

    %typemap(csvarout, excode=SWIGEXCODE2) CPP_TYPE &, CPP_TYPE * %{
        get {
            var res = $imcall;$excode
            return res;
        }
    %}

    // Ptr types
    %apply       CPP_TYPE & {       CPP_TYPE * };
    %apply const CPP_TYPE & { const CPP_TYPE * };
%enddef

%define URHO3D_BINARY_COMPATIBLE_TYPE(TYPE, POD_TYPE)
    URHO3D_BINARY_COMPATIBLE_TYPE_EX(TYPE, TYPE, POD_TYPE);
%enddef
