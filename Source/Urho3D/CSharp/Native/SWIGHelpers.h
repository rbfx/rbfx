#include <cstddef>

template<typename T> T* addr(T& ref)  { return &ref; }
template<typename T> T* addr(T* ptr)  { return ptr;  }
template<typename T> T* addr(SwigValueWrapper<T>& ref)  { return &(T&)ref; }
template<typename T> T* addr(SwigValueWrapper<T>* ptr)  { return *ptr;  }
template<typename T> T& deref(T& ref) { return ref;  }
template<typename T> T& deref(T* ptr) { return *ptr; }
template<typename T> T& deref(SwigValueWrapper<T>& ref) { return (T&)ref;  }
template<typename T> T& deref(SwigValueWrapper<T>* ptr) { return (T&)*ptr; }

namespace pod
{

#define DEFINE_POD_HELPER_STRUCT(type, n) struct type##n { type data[n]; };
DEFINE_POD_HELPER_STRUCT(int, 2);
DEFINE_POD_HELPER_STRUCT(int, 3);
DEFINE_POD_HELPER_STRUCT(int, 4);
DEFINE_POD_HELPER_STRUCT(float, 2);
DEFINE_POD_HELPER_STRUCT(float, 3);
DEFINE_POD_HELPER_STRUCT(float, 4);
DEFINE_POD_HELPER_STRUCT(float, 6);
DEFINE_POD_HELPER_STRUCT(float, 7);
DEFINE_POD_HELPER_STRUCT(float, 8);
DEFINE_POD_HELPER_STRUCT(float, 9);
DEFINE_POD_HELPER_STRUCT(float, 12);
DEFINE_POD_HELPER_STRUCT(float, 16);
#undef DEFINE_POD_HELPER_STRUCT

template<typename From, typename To>
To convert(const From& from)
{
    static_assert(sizeof(From) == sizeof(To), "");
    To value;
    memcpy(&value, &from, sizeof(from));
    return value;
}

}
