
%define URHO3D_VECTOR_TEMPLATE_INTERNAL(VECTOR, CSINTERFACE, CONST_REFERENCE, CTYPE...)

%typemap(csinterfaces) Urho3D::VECTOR< CTYPE > "global::System.IDisposable, global::System.Collections.IEnumerable, global::System.Collections.Generic.CSINTERFACE<$typemap(cstype, CTYPE)>\n";
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
    VECTOR();
    VECTOR(const VECTOR& other);
    %extend {
      VECTOR(int capacity) {
        auto* pv = new VECTOR< CTYPE >();
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
      void AddRange(const VECTOR< CTYPE >& values) {
        $self->Push(values);
      }
      // Takes a deep copy of the elements unlike ArrayList.GetRange
      VECTOR< CTYPE > *GetRange(int index, int count) throw (std::out_of_range, std::invalid_argument) {
        if (index < 0)
          throw std::out_of_range("index");
        if (count < 0)
          throw std::out_of_range("count");
        if (index >= (int)$self->Size() + 1 || index + count > (int)$self->Size())
          throw std::invalid_argument("invalid range");
        auto* result = new VECTOR< CTYPE >();
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
      void InsertRange(int index, const VECTOR< CTYPE >& values) throw (std::out_of_range) {
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

    %csmethodmodifiers VECTOR::getitemcopy "private"
    %csmethodmodifiers VECTOR::getitem "private"
    %csmethodmodifiers VECTOR::setitem "private"
    %csmethodmodifiers VECTOR::size "private"
    %csmethodmodifiers VECTOR::capacity "private"
    %csmethodmodifiers VECTOR::reserve "private"
%enddef


namespace Urho3D {
    template<class T> class PODVector {
        URHO3D_VECTOR_TEMPLATE_INTERNAL(PODVector, IList, T const&, T)
    };
    template<class T> class Vector {
        URHO3D_VECTOR_TEMPLATE_INTERNAL(Vector, IList, T const&, T)
    };
}


%template(StringHashArray) Urho3D::PODVector<Urho3D::StringHash>;
%template(Vector2Array)    Urho3D::PODVector<Urho3D::Vector2>;
%template(Vector3Array)    Urho3D::PODVector<Urho3D::Vector3>;
%template(Vector4Array)    Urho3D::PODVector<Urho3D::Vector4>;
%template(IntVector2Array) Urho3D::PODVector<Urho3D::IntVector2>;
%template(IntVector3Array) Urho3D::PODVector<Urho3D::IntVector3>;
%template(QuaternionArray) Urho3D::PODVector<Urho3D::Quaternion>;
%template(RectArray)       Urho3D::PODVector<Urho3D::Rect>;
%template(IntRectArray)    Urho3D::PODVector<Urho3D::IntRect>;
%template(Matrix3x4Array)  Urho3D::PODVector<Urho3D::Matrix3x4>;
%template(BoolArray)       Urho3D::PODVector<bool>;
%template(CharArray)       Urho3D::PODVector<char>;
%template(ShortArray)      Urho3D::PODVector<short>;
%template(IntArray)        Urho3D::PODVector<int>;
%template(UCharArray)      Urho3D::PODVector<unsigned char>;
%template(UShortArray)     Urho3D::PODVector<unsigned short>;
%template(UIntArray)       Urho3D::PODVector<unsigned int>;
%template(FloatArray)      Urho3D::PODVector<float>;
%template(DoubleArray)     Urho3D::PODVector<double>;

%template(ObjectArray)           Urho3D::PODVector<Urho3D::Object*>;
%template(SoundSourceArray)      Urho3D::PODVector<Urho3D::SoundSource*>;
//%template(BatchArray)            Urho3D::PODVector<Urho3D::Batch*>;
//%template(BatchGroupArray)       Urho3D::PODVector<Urho3D::BatchGroup*>;
%template(ComponentArray)        Urho3D::PODVector<Urho3D::Component*>;
%template(DrawableArray)         Urho3D::PODVector<Urho3D::Drawable*>;
%template(ImageArray)            Urho3D::PODVector<Urho3D::Image*>;
%template(LightArray)            Urho3D::PODVector<Urho3D::Light*>;
%template(NodeArray)             Urho3D::PODVector<Urho3D::Node*>;
%template(PassArray)             Urho3D::PODVector<Urho3D::Pass*>;
%template(ReplicationStateArray) Urho3D::PODVector<Urho3D::ReplicationState*>;
%template(ResourceArray)         Urho3D::PODVector<Urho3D::Resource*>;
//%template(RigidBodyArray)        Urho3D::PODVector<Urho3D::RigidBody*>;
%template(UIElementArray)        Urho3D::PODVector<Urho3D::UIElement*>;
%template(VAnimEventFrameArray)  Urho3D::PODVector<const Urho3D::VAnimEventFrame*>;
%template(VertexElementArray)    Urho3D::PODVector<Urho3D::VertexElement>;
%template(VertexBufferArray)     Urho3D::PODVector<Urho3D::VertexBuffer*>;
%template(IndexBufferArray)      Urho3D::PODVector<Urho3D::IndexBuffer*>;
%template(BillboardArray)        Urho3D::PODVector<Urho3D::Billboard>;
%template(DecalVertexArray)      Urho3D::PODVector<Urho3D::DecalVertex>;
%template(CustomGeometryVerticesArray) Urho3D::PODVector<Urho3D::CustomGeometryVertex>;
%template(RayQueryResultArray)   Urho3D::PODVector<Urho3D::RayQueryResult>;
%template(SourceBatchVector)     Urho3D::Vector<Urho3D::SourceBatch>;
%template(CameraArray)           Urho3D::PODVector<Urho3D::Camera*>;

%template(StringVector)      Urho3D::Vector<Urho3D::String>;
%template(VariantVector)     Urho3D::Vector<Urho3D::Variant>;
%template(AttributeInfoVector) Urho3D::Vector<Urho3D::AttributeInfo>;
%template(JSONArray)         Urho3D::Vector<Urho3D::JSONValue>;
%template(PListValueVector)  Urho3D::Vector<Urho3D::PListValue>;
%template(PackageFileVector) Urho3D::Vector<Urho3D::SharedPtr<Urho3D::PackageFile>>;
%template(ComponentVector)   Urho3D::Vector<Urho3D::SharedPtr<Urho3D::Component>>;
%template(NodeVector)        Urho3D::Vector<Urho3D::SharedPtr<Urho3D::Node>>;
%template(UIElementVector)   Urho3D::Vector<Urho3D::SharedPtr<Urho3D::UIElement>>;
%template(Texture2DVector)   Urho3D::Vector<Urho3D::SharedPtr<Urho3D::Texture2D>>;
%template(ComponentVector2)  Urho3D::Vector<Urho3D::WeakPtr<Urho3D::Component>>;
//%template(VAnimKeyFrameVector) Urho3D::Vector<Urho3D::VAnimKeyFrame>; // some issue with const
%template(GeometryVector)          Urho3D::Vector<Urho3D::SharedPtr<Urho3D::Geometry>>;
%template(VertexBufferVector)      Urho3D::Vector<Urho3D::SharedPtr<Urho3D::VertexBuffer>>;
%template(IndexBufferVector)       Urho3D::Vector<Urho3D::SharedPtr<Urho3D::IndexBuffer>>;
//%template(ConnectionVector) Urho3D::Vector<Urho3D::SharedPtr<Urho3D::Connection>>;
%template(GeometriesVector)        Urho3D::Vector<Urho3D::Vector<Urho3D::SharedPtr<Urho3D::Geometry>>>;
%template(RenderPathCommandVector) Urho3D::Vector<Urho3D::RenderPathCommand>;
%template(RenderTargetInfoVector)  Urho3D::Vector<Urho3D::RenderTargetInfo>;
%template(BonesVector)             Urho3D::Vector<Urho3D::Bone>;
%template(AnimationControlVector)  Urho3D::Vector<Urho3D::AnimationControl>;
%template(ModelMorphVector)        Urho3D::Vector<Urho3D::ModelMorph>;
%template(AnimationStateVector)    Urho3D::Vector<Urho3D::SharedPtr<Urho3D::AnimationState>>;
%template(UIntArrayVector)         Urho3D::Vector<Urho3D::PODVector<unsigned int>>;
%template(Matrix3x4ArrayVector)    Urho3D::Vector<Urho3D::PODVector<Urho3D::Matrix3x4>>;
%template(AnimationKeyFrameVector) Urho3D::Vector<Urho3D::AnimationKeyFrame>;
%template(AnimationTrackVector)    Urho3D::Vector<Urho3D::AnimationTrack>;
%template(AnimationTriggerPointVector) Urho3D::Vector<Urho3D::AnimationTriggerPoint>;
%template(ShaderVariationVector)    Urho3D::Vector<Urho3D::SharedPtr<Urho3D::ShaderVariation>>;
%template(ColorFrameVector)         Urho3D::Vector<Urho3D::ColorFrame>;
%template(TextureFrameVector)       Urho3D::Vector<Urho3D::TextureFrame>;
%template(TechniqueEntryVector)     Urho3D::Vector<Urho3D::TechniqueEntry>;
%template(CustomGeometryVerticesVector) Urho3D::Vector<Urho3D::PODVector<Urho3D::CustomGeometryVertex>>;
