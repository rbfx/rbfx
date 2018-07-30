
namespace Urho3D { class PODVector; }
// ----------------------------------- PODVector<builtin> -----------------------------------
%define URHO3D_PODVECTOR_ARRAY(CTYPE, CSTYPE)
  %typemap(ctype)  Urho3D::PODVector<CTYPE> & "::SafeArray"                          // c layer type
  %typemap(imtype) Urho3D::PODVector<CTYPE> & "global::Urho3DNet.Urho3D.SafeArray"   // pinvoke type
  %typemap(cstype) Urho3D::PODVector<CTYPE> & "CSTYPE[]"                             // c# type

  // c to cpp
  %typemap(in)     Urho3D::PODVector<CTYPE> & %{
      Urho3D::PODVector<CTYPE> $1_tmp((const CTYPE*)$input.data, $input.length);
      $1 = &$1_tmp;
  %}

  // cpp to c
  %typemap(out)    Urho3D::PODVector<CTYPE> & %{ 
    $result = ::SafeArray{(void*)&$1->Front(), (int)$1->Size()};
  %}

  // C# to pinvoke
  %typemap(csin,   pre=         "    unsafe{fixed (CSTYPE* swig_ptrTo_$csinput = $csinput) {",
                   terminator = "    }}") 
                   Urho3D::PODVector<CTYPE> & %{
    new global::Urho3DNet.Urho3D.SafeArray((global::System.IntPtr)swig_ptrTo_$csinput, $csinput.Length)
  %}

    // pinvoke to C#
  %typemap(csout, excode=SWIGEXCODE) Urho3D::PODVector<CTYPE> & {                    // convert pinvoke to C#
    var ret = $imcall;$excode
    var res = new CSTYPE[ret.length];
    unsafe {
      fixed (CSTYPE* pRes = res) {
        var len = ret.length * global::System.Runtime.InteropServices.Marshal.SizeOf<CSTYPE>();
        global::System.Buffer.MemoryCopy((void*)ret.data, (void*)pRes, len, len);
      }
    }
    return res;
  }

  %typemap(csvarin, excode=SWIGEXCODE2) Urho3D::PODVector<CTYPE> & %{
    set {
      unsafe {
        fixed (CSTYPE* swig_ptrTo_$csinput = $csinput) {
          $imcall;$excode
        }
      }
    }
  %}
  %typemap(csvarout, excode=SWIGEXCODE2) Urho3D::PODVector<CTYPE> & %{
    get { $typemap(csout, Urho3D::PODVector<CTYPE> &) }
  %}
%enddef

URHO3D_PODVECTOR_ARRAY(Urho3D::StringHash, global::Urho3DNet.StringHash);
URHO3D_PODVECTOR_ARRAY(unsigned char, byte);
URHO3D_PODVECTOR_ARRAY(float, float);
URHO3D_PODVECTOR_ARRAY(bool, bool);
URHO3D_PODVECTOR_ARRAY(int, int);

// ----------------------------------- PODVector<T*> -----------------------------------
%define URHO3D_PODVECTOR_PTR_ARRAY(CTYPE, CSTYPE)
  %typemap(ctype)  Urho3D::PODVector<CTYPE*> & "::SafeArray"                          // c layer type
  %typemap(imtype) Urho3D::PODVector<CTYPE*> & "global::Urho3DNet.Urho3D.SafeArray"   // pinvoke type
  %typemap(cstype) Urho3D::PODVector<CTYPE*> & "CSTYPE[]"                             // c# type

  // c to cpp
   %typemap(in)     Urho3D::PODVector<CTYPE*> & %{
      Urho3D::PODVector<CTYPE*> $1_tmp((CTYPE* const*)$input.data, (unsigned)$input.length);
      $1 = &$1_tmp;
  %}

  // cpp to c
  %typemap(out)    Urho3D::PODVector<CTYPE*> & %{ 
    $result = ::SafeArray{(void*)&$1->Front(), (int)$1->Size()};
  %}

  // C# to pinvoke
  %typemap(csin,   pre=         "
    var $csinput_ptr_array = new global::System.IntPtr[$csinput.Length];
    for (var i = 0; i < $csinput.Length; i++)
      $csinput_ptr_array[i] = CSTYPE.getCPtr($csinput[i]).Handle;
    unsafe {
      fixed (CSTYPE* p_$csinput = $csinput) {
  ", terminator = "    } }") Urho3D::PODVector<CTYPE*> & "new global::Urho3DNet.Urho3D.SafeArray((global::System.IntPtr)p_$csinput, $csinput.Length)"

  // pinvoke to C#
  %typemap(csout, excode=SWIGEXCODE) Urho3D::PODVector<CTYPE*> & {                    // convert pinvoke to C#
    var $n_name_ret = $imcall;$excode
    var $n_name_res = new CSTYPE[ret.length];
    unsafe {
      fixed (global::System.IntPtr* p$n_name_Ret = $n_name_ret) {
        for (var i = 0; i < $n_name_ret.length; i++)
          $n_name_res[i] = new CSTYPE(p$n_name_Ret[i], false);
      }
    }
    return res;
  }

  %typemap(csvarin, excode=SWIGEXCODE2) Urho3D::PODVector<CTYPE*> & %{
    set {
      var $csinput_ptr_array = new global::System.IntPtr[$csinput.Length];
      for (var i = 0; i < $csinput.Length; i++)
        $csinput_ptr_array[i] = CSTYPE.getCPtr($csinput[i]).Handle;
      unsafe {
        fixed (global::System.IntPtr* p_$csinput = $csinput_ptr_array) {
          $imcall;$excode
        }
      }
    }
  %}
  %typemap(csvarout, excode=SWIGEXCODE2) Urho3D::PODVector<CTYPE*> & %{
    get {
      var ret = $imcall;$excode
      var res = new CSTYPE[ret.length];
      unsafe {
        global::System.IntPtr* pRet = &ret.data;
        for (var i = 0; i < ret.length; i++)
          res[i] = new CSTYPE(pRet[i], false);
      }
      return res;
    }
  %}
%enddef

URHO3D_PODVECTOR_PTR_ARRAY(Urho3D::Object, global::Urho3DNet.Object);







%define URHO3D_VECTOR_TEMPLATE_INTERNAL(CSINTERFACE, CONST_REFERENCE, CTYPE...)

%typemap(csinterfaces) Urho3D::Vector< CTYPE > "global::System.IDisposable, global::System.Collections.IEnumerable, global::System.Collections.Generic.CSINTERFACE<$typemap(cstype, CTYPE)>\n";
%proxycode %{
  public $csclassname(global::System.Collections.ICollection c) : this() {
    if (c == null)
      throw new global::System.ArgumentNullException("c");
    foreach ($typemap(cstype, CTYPE) element in c) {
      this.Add(element);
    }
  }

  public bool IsFixedSize => false;
  public bool IsReadOnly => false;

  public $typemap(cstype, CTYPE) this[int index]  {
    get {
      return getitem(index);
    }
    set {
      setitem(index, value);
    }
  }

  public int Capacity {
    get {
      return (int)capacity();
    }
    set {
      if (value < size())
        throw new global::System.ArgumentOutOfRangeException("Capacity");
      reserve((uint)value);
    }
  }

  public int Count {
    get {
      return (int)size();
    }
  }

  public bool IsSynchronized => false;

  public void CopyTo($typemap(cstype, CTYPE)[] array)
  {
    CopyTo(0, array, 0, this.Count);
  }

  public void CopyTo($typemap(cstype, CTYPE)[] array, int arrayIndex)
  {
    CopyTo(0, array, arrayIndex, this.Count);
  }

  public void CopyTo(int index, $typemap(cstype, CTYPE)[] array, int arrayIndex, int count)
  {
    if (array == null)
      throw new global::System.ArgumentNullException("array");
    if (index < 0)
      throw new global::System.ArgumentOutOfRangeException("index", "Value is less than zero");
    if (arrayIndex < 0)
      throw new global::System.ArgumentOutOfRangeException("arrayIndex", "Value is less than zero");
    if (count < 0)
      throw new global::System.ArgumentOutOfRangeException("count", "Value is less than zero");
    if (array.Rank > 1)
      throw new global::System.ArgumentException("Multi dimensional array.", "array");
    if (index+count > this.Count || arrayIndex+count > array.Length)
      throw new global::System.ArgumentException("Number of elements to copy is too large.");
    for (int i=0; i<count; i++)
      array.SetValue(getitemcopy(index+i), arrayIndex+i);
  }

  global::System.Collections.Generic.IEnumerator<$typemap(cstype, CTYPE)> global::System.Collections.Generic.IEnumerable<$typemap(cstype, CTYPE)>.GetEnumerator() {
    return new $csclassnameEnumerator(this);
  }

  global::System.Collections.IEnumerator global::System.Collections.IEnumerable.GetEnumerator() {
    return new $csclassnameEnumerator(this);
  }

  public $csclassnameEnumerator GetEnumerator() {
    return new $csclassnameEnumerator(this);
  }

  // Type-safe enumerator
  /// Note that the IEnumerator documentation requires an InvalidOperationException to be thrown
  /// whenever the collection is modified. This has been done for changes in the size of the
  /// collection but not when one of the elements of the collection is modified as it is a bit
  /// tricky to detect unmanaged code that modifies the collection under our feet.
  public sealed class $csclassnameEnumerator : global::System.Collections.IEnumerator
    , global::System.Collections.Generic.IEnumerator<$typemap(cstype, CTYPE)>
  {
    private $csclassname collectionRef;
    private int currentIndex;
    private object currentObject;
    private int currentSize;

    public $csclassnameEnumerator($csclassname collection) {
      collectionRef = collection;
      currentIndex = -1;
      currentObject = null;
      currentSize = collectionRef.Count;
    }

    // Type-safe iterator Current
    public $typemap(cstype, CTYPE) Current {
      get {
        if (currentIndex == -1)
          throw new global::System.InvalidOperationException("Enumeration not started.");
        if (currentIndex > currentSize - 1)
          throw new global::System.InvalidOperationException("Enumeration finished.");
        if (currentObject == null)
          throw new global::System.InvalidOperationException("Collection modified.");
        return ($typemap(cstype, CTYPE))currentObject;
      }
    }

    // Type-unsafe IEnumerator.Current
    object global::System.Collections.IEnumerator.Current {
      get {
        return Current;
      }
    }

    public bool MoveNext() {
      int size = collectionRef.Count;
      bool moveOkay = (currentIndex+1 < size) && (size == currentSize);
      if (moveOkay) {
        currentIndex++;
        currentObject = collectionRef[currentIndex];
      } else {
        currentObject = null;
      }
      return moveOkay;
    }

    public void Reset() {
      currentIndex = -1;
      currentObject = null;
      if (collectionRef.Count != currentSize) {
        throw new global::System.InvalidOperationException("Collection modified.");
      }
    }

    public void Dispose() {
        currentIndex = -1;
        currentObject = null;
    }
  }
%}

  public:
    void Clear();
    %rename(Add) Push;
    void Push(const CTYPE& x);
    bool Contains(const CTYPE& x);
    bool Remove(const CTYPE& x);
    int IndexOf(const CTYPE& x);
    %rename(size) Size;
    unsigned Size() const;
    %rename(capacity) Capacity;
    unsigned Capacity() const;
    %rename(reserve) Reserve;
    void Reserve(unsigned n);
    %newobject GetRange(int index, int count);
    Vector();
    Vector(Vector other);
    %extend {
      Vector(int capacity) {
        auto* pv = new Urho3D::Vector< CTYPE >();
        pv->Reserve(capacity);
        return pv;
      }
      CTYPE getitemcopy(int index) throw (std::out_of_range) {
        if (index < 0 || (unsigned)index >= $self->Size())
          throw std::out_of_range("index");
        return (*$self)[index];
      }
      const CTYPE& getitem(int index) throw (std::out_of_range) {
        if (index < 0 || (unsigned)index >= $self->Size())
          throw std::out_of_range("index");
        return (*$self)[index];
      }
      void setitem(int index, const CTYPE& val) throw (std::out_of_range) {
        if (index < 0 || (unsigned)index >= $self->Size())
          throw std::out_of_range("index");
        (*$self)[index] = val;
      }
      // Takes a deep copy of the elements unlike ArrayList.AddRange
      void AddRange(const Urho3D::Vector< CTYPE >& values) {
        $self->Push(values);
      }
      // Takes a deep copy of the elements unlike ArrayList.GetRange
      Urho3D::Vector< CTYPE > *GetRange(int index, int count) throw (std::out_of_range, std::invalid_argument) {
        if (index < 0)
          throw std::out_of_range("index");
        if (count < 0)
          throw std::out_of_range("count");
        if (index >= (int)$self->Size() + 1 || index + count > (int)$self->Size())
          throw std::invalid_argument("invalid range");
        auto* result = new Urho3D::Vector< CTYPE >();
        result->Insert(result->End(), $self->Begin() + index, $self->Begin() + index + count);
        return result;
      }
      void Insert(int index, const CTYPE& x) throw (std::out_of_range) {
        if (index >= 0 && index < (int)$self->Size() + 1)
          $self->Insert($self->Begin() + index, x);
        else
          throw std::out_of_range("index");
      }
      // Takes a deep copy of the elements unlike ArrayList.InsertRange
      void InsertRange(int index, const Urho3D::Vector< CTYPE >& values) throw (std::out_of_range) {
        if (index >= 0 && index < (int)$self->Size() + 1)
          $self->Insert($self->Begin() + index, values.Begin(), values.End());
        else
          throw std::out_of_range("index");
      }
      void RemoveAt(int index) throw (std::out_of_range) {
        if (index >= 0 && index < (int)$self->Size())
          $self->Erase($self->Begin() + index);
        else
          throw std::out_of_range("index");
      }
      void RemoveRange(int index, int count) throw (std::out_of_range, std::invalid_argument) {
        if (index < 0)
          throw std::out_of_range("index");
        if (count < 0)
          throw std::out_of_range("count");
        if (index >= (int)$self->Size() + 1 || index + count > (int)$self->Size())
          throw std::invalid_argument("invalid range");
        $self->Erase($self->Begin() + index, $self->Begin() + index + count);
      }
    }

%enddef

%csmethodmodifiers Urho3D::Vector::getitemcopy "private"
%csmethodmodifiers Urho3D::Vector::getitem "private"
%csmethodmodifiers Urho3D::Vector::setitem "private"
%csmethodmodifiers Urho3D::Vector::size "private"
%csmethodmodifiers Urho3D::Vector::capacity "private"
%csmethodmodifiers Urho3D::Vector::reserve "private"

namespace Urho3D {
  template<class T> class Vector {
    URHO3D_VECTOR_TEMPLATE_INTERNAL(IList, T const&, T)
  };
}

%template(StringVector)     Urho3D::Vector<Urho3D::String>;
%template(VariantVector)    Urho3D::Vector<Urho3D::Variant>;
