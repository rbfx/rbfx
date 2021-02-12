
// Defined in C#
namespace Urho3D {

class StringHash;

// StringHash
%typemap(ctype)  StringHash "unsigned"                                // c layer type
%typemap(imtype) StringHash "uint"                                    // pinvoke type
%typemap(cstype) StringHash "StringHash"                              // c# type
%typemap(in)     StringHash "$1 = StringHash($input);"                // c to cpp
%typemap(out)    StringHash "$result = $1.Value();"                   // cpp to c
%typemap(csin)   StringHash "$csinput.Hash"                           // convert C# to pinvoke
%typemap(csout, excode=SWIGEXCODE) StringHash {                       // convert pinvoke to C#
    var ret = new $typemap(cstype, Urho3D::StringHash)($imcall);$excode
    return ret;
}
%typemap(directorin)    StringHash "$input = $1.Value();"
%typemap(directorout)   StringHash "$result = ($1_ltype)$input;"
%typemap(csdirectorin)  StringHash "new $typemap(cstype, Urho3D::StringHash)($iminput)"
%typemap(csdirectorout) StringHash "$cscall.Hash"

%typemap(csvarin, excode=SWIGEXCODE2) StringHash & %{
    set {
        $imcall;$excode
    }
%}
%typemap(csvarout, excode=SWIGEXCODE2) StringHash & %{
    get {
        var ret = new $typemap(cstype, Urho3D::StringHash)($imcall);$excode
        return ret;
    }
%}

%apply StringHash { StringHash & }
%typemap(in) StringHash &  %{
    $*1_ltype $1_tmp($input);
    $1 = &$1_tmp;
%}
%typemap(out) StringHash & %{ $result = $1->Value(); %}               // cpp to c

}
