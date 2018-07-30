// Based on swig's std_map.i
%define URHO3D_HASHMAP_INTERNAL(K, T)

%typemap(csinterfaces) Urho3D::HashMap< K, T > "global::System.IDisposable \n    , global::System.Collections.Generic.IDictionary<$typemap(cstype, K), $typemap(cstype, T)>\n";
%proxycode %{

  public $typemap(cstype, T) this[$typemap(cstype, K) key] {
    get {
      return getitem(key);
    }

    set {
      setitem(key, value);
    }
  }

  public bool TryGetValue($typemap(cstype, K) key, out $typemap(cstype, T) value) {
    if (this.ContainsKey(key)) {
      value = this[key];
      return true;
    }
    value = default($typemap(cstype, T));
    return false;
  }

  public int Count {
    get {
      return (int)Size();
    }
  }

  public bool IsReadOnly {
    get { 
      return false; 
    }
  }

  public global::System.Collections.Generic.ICollection<$typemap(cstype, K)> Keys {
    get {
      global::System.Collections.Generic.ICollection<$typemap(cstype, K)> keys = new global::System.Collections.Generic.List<$typemap(cstype, K)>();
      int size = this.Count;
      if (size > 0) {
        global::System.IntPtr iter = create_iterator_begin();
        for (int i = 0; i < size; i++) {
          keys.Add(get_next_key(iter));
        }
      }
      return keys;
    }
  }

  public global::System.Collections.Generic.ICollection<$typemap(cstype, T)> Values {
    get {
      global::System.Collections.Generic.ICollection<$typemap(cstype, T)> vals = new global::System.Collections.Generic.List<$typemap(cstype, T)>();
      foreach (global::System.Collections.Generic.KeyValuePair<$typemap(cstype, K), $typemap(cstype, T)> pair in this) {
        vals.Add(pair.Value);
      }
      return vals;
    }
  }
  
  public void Add(global::System.Collections.Generic.KeyValuePair<$typemap(cstype, K), $typemap(cstype, T)> item) {
    Add(item.Key, item.Value);
  }

  public bool Remove(global::System.Collections.Generic.KeyValuePair<$typemap(cstype, K), $typemap(cstype, T)> item) {
    if (Contains(item)) {
      return Remove(item.Key);
    } else {
      return false;
    }
  }

  public bool Contains(global::System.Collections.Generic.KeyValuePair<$typemap(cstype, K), $typemap(cstype, T)> item) {
    if (this[item.Key] == item.Value) {
      return true;
    } else {
      return false;
    }
  }

  public void CopyTo(global::System.Collections.Generic.KeyValuePair<$typemap(cstype, K), $typemap(cstype, T)>[] array) {
    CopyTo(array, 0);
  }

  public void CopyTo(global::System.Collections.Generic.KeyValuePair<$typemap(cstype, K), $typemap(cstype, T)>[] array, int arrayIndex) {
    if (array == null)
      throw new global::System.ArgumentNullException("array");
    if (arrayIndex < 0)
      throw new global::System.ArgumentOutOfRangeException("arrayIndex", "Value is less than zero");
    if (array.Rank > 1)
      throw new global::System.ArgumentException("Multi dimensional array.", "array");
    if (arrayIndex+this.Count > array.Length)
      throw new global::System.ArgumentException("Number of elements to copy is too large.");

    global::System.Collections.Generic.IList<$typemap(cstype, K)> keyList = new global::System.Collections.Generic.List<$typemap(cstype, K)>(this.Keys);
    for (int i = 0; i < keyList.Count; i++) {
      $typemap(cstype, K) currentKey = keyList[i];
      array.SetValue(new global::System.Collections.Generic.KeyValuePair<$typemap(cstype, K), $typemap(cstype, T)>(currentKey, this[currentKey]), arrayIndex+i);
    }
  }

  global::System.Collections.Generic.IEnumerator<global::System.Collections.Generic.KeyValuePair<$typemap(cstype, K), $typemap(cstype, T)>> global::System.Collections.Generic.IEnumerable<global::System.Collections.Generic.KeyValuePair<$typemap(cstype, K), $typemap(cstype, T)>>.GetEnumerator() {
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
  public sealed class $csclassnameEnumerator : global::System.Collections.IEnumerator, 
      global::System.Collections.Generic.IEnumerator<global::System.Collections.Generic.KeyValuePair<$typemap(cstype, K), $typemap(cstype, T)>>
  {
    private $csclassname collectionRef;
    private global::System.Collections.Generic.IList<$typemap(cstype, K)> keyCollection;
    private int currentIndex;
    private object currentObject;
    private int currentSize;

    public $csclassnameEnumerator($csclassname collection) {
      collectionRef = collection;
      keyCollection = new global::System.Collections.Generic.List<$typemap(cstype, K)>(collection.Keys);
      currentIndex = -1;
      currentObject = null;
      currentSize = collectionRef.Count;
    }

    // Type-safe iterator Current
    public global::System.Collections.Generic.KeyValuePair<$typemap(cstype, K), $typemap(cstype, T)> Current {
      get {
        if (currentIndex == -1)
          throw new global::System.InvalidOperationException("Enumeration not started.");
        if (currentIndex > currentSize - 1)
          throw new global::System.InvalidOperationException("Enumeration finished.");
        if (currentObject == null)
          throw new global::System.InvalidOperationException("Collection modified.");
        return (global::System.Collections.Generic.KeyValuePair<$typemap(cstype, K), $typemap(cstype, T)>)currentObject;
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
        $typemap(cstype, K) currentKey = keyCollection[currentIndex];
        currentObject = new global::System.Collections.Generic.KeyValuePair<$typemap(cstype, K), $typemap(cstype, T)>(currentKey, collectionRef[currentKey]);
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
    HashMap();
    HashMap(HashMap< K, T > other);
    unsigned Size() const;
    bool Empty() const;
    void Clear();
    %extend {
      T getitem(const K& key) throw (std::out_of_range) {
        auto iter = $self->Find(key);
        if (iter != $self->End())
          return iter->second_;
        else
          throw std::out_of_range("key not found");
      }

      void setitem(const K& key, const T& x) {
        (*$self)[key] = x;
      }

      bool ContainsKey(const K& key) {
        return $self->Find(key) != $self->End();
      }

      void Add(const K& key, const T& val) throw (std::out_of_range) {
        auto iter = $self->Find(key);
        if (iter != $self->End())
          throw std::out_of_range("key already exists");
        (*$self)[key] = val;
      }

      bool Remove(const K& key) {
        auto iter = $self->Find(key);
        if (iter != $self->End()) {
          $self->Erase(iter);
          return true;
        }                
        return false;
      }

      // create_iterator_begin(), get_next_key() and destroy_iterator work together to provide a collection of keys to C#
      //%apply void *VOID_INT_PTR { Urho3D::HashMap< K, T >::Iterator* create_iterator_begin }
      //%apply void *VOID_INT_PTR { Urho3D::HashMap< K, T >::Iterator* swigiterator }

      void* create_iterator_begin() {
        return (void*)$self->Begin().ptr_;
      }

      K get_next_key(void* swigiterator) {
        Urho3D::HashMap< K, T >::Iterator iter((Urho3D::HashMap< K, T >::Node*)swigiterator);
        auto&& result = iter->first_;
        iter++;
        return result;
      }
    }


%enddef

%csmethodmodifiers Urho3D::HashMap::size "private"
%csmethodmodifiers Urho3D::HashMap::getitem "private"
%csmethodmodifiers Urho3D::HashMap::setitem "private"
%csmethodmodifiers Urho3D::HashMap::create_iterator_begin "private"
%csmethodmodifiers Urho3D::HashMap::get_next_key "private"

// Default implementation
namespace Urho3D {   
  template<class K, class T > class HashMap {
    URHO3D_HASHMAP_INTERNAL(K, T)
  };
}

%template(VariantMap) Urho3D::HashMap<Urho3D::StringHash, Urho3D::Variant>;
