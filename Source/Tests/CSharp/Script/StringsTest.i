
///////////////////////////////////////////////////////////////////////////////
%director CStringTestHelper;
%inline
%{

class CStringTestHelper
{
public:
    CStringTestHelper()
        : value_("value")
    {
    }

    const char* testMethod(const char* p)
    {
        storage_ = Format("p: {}", p);
        return storage_.c_str();
    }

    virtual const char* testVirtualMethod(const char* p)
    {
        storage_ = Format("v: {}", p);
        return storage_.c_str();
    }

    const char* value_;

private:
    ea::string storage_;
};

///////////////////////////////////////////////////////////////////////////////

template<typename T>
class StringTestHelperRO
{
public:
    StringTestHelperRO()
        : value_("value")
        , valueConstRef_(value_)
    {
    }

    virtual ~StringTestHelperRO() = default;

    T testMethod(T p)
    {
        storage_ = Format("p: {}", p);
        return T(storage_.data(), storage_.size());
    }

    virtual T testVirtualMethod(T p)
    {
        storage_ = Format("v: {}", p);
        return T(storage_.data(), storage_.size());
    }

    const T& testMethodConstRef(const T& p)
    {
        storage_ = Format("p: {}", p);
        storageView_ = T(storage_.data(), storage_.size());
        return storageView_;
    }

    virtual const T& testVirtualMethodConstRef(const T& p)
    {
        storage_ = Format("v: {}", p);
        storageView_ = T(storage_.data(), storage_.size());
        return storageView_;
    }

    bool testVirtuals()
    {
        if (testVirtualMethod("foo").compare("o: foo") != 0)
            return false;
        if (testVirtualMethodConstRef("foo").compare("o: foo") != 0)
            return false;
        return true;
    }

    T value_;
    const T& valueConstRef_;

private:
    ea::string storage_;
    T storageView_;
};

%}

%director StringTestHelperRO<eastl::string_view>;
%director StringTestHelperRO<std::string_view>;
%director StringTestHelperRO<eastl::string>;
%director StringTestHelperRO<std::string>;
%template(EaStringViewTestHelper)   StringTestHelperRO<eastl::string_view>;
%template(StdStringViewTestHelper)  StringTestHelperRO<std::string_view>;
%template(EaStringTestHelperRO)     StringTestHelperRO<eastl::string>;
%template(StdStringTestHelperRO)    StringTestHelperRO<std::string>;

///////////////////////////////////////////////////////////////////////////////

%inline
%{

template<typename T>
class StringTestHelperRW : public StringTestHelperRO<T>
{
public:
    StringTestHelperRW()
		: StringTestHelperRO<T>()
        , valueRef_(StringTestHelperRO<T>::value_)
    {
    }

    void doubleStringRef(T& str)
    {
        str = Format("{}+{}", str, str).data();
    }

    void doubleStringPtr(T* str)
    {
        if (str == nullptr)
        {
            return;
        }
        *str = Format("{}+{}", *str, *str).data();
    }

    T& valueRef_;
};

%}

%director StringTestHelperRW<eastl::string>;
%director StringTestHelperRW<std::string>;
%template(EaStringTestHelper)  StringTestHelperRW<eastl::string>;
%template(StdStringTestHelper) StringTestHelperRW<std::string>;
