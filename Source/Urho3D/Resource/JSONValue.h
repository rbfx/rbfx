//
// Copyright (c) 2008-2019 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include <EASTL/map.h>

#include "../Core/Variant.h"

namespace Urho3D
{

class Context;

/// JSON value type.
enum JSONValueType
{
    /// JSON null type.
    JSON_NULL = 0,
    /// JSON boolean type.
    JSON_BOOL,
    /// JSON number type.
    JSON_NUMBER,
    /// JSON string type.
    JSON_STRING,
    /// JSON array type.
    JSON_ARRAY,
    /// JSON object type.
    JSON_OBJECT
};

/// JSON number type.
enum JSONNumberType
{
    /// Not a number.
    JSONNT_NAN = 0,
    /// Integer.
    JSONNT_INT,
    /// Unsigned integer.
    JSONNT_UINT,
    /// Float or double.
    JSONNT_FLOAT_DOUBLE
};

class JSONValue;

/// JSON value class.
class URHO3D_API JSONValue
{
public:
    /// Construct null value.
    JSONValue() :
        type_(0)
    {
    }

    /// Construct a default value with defined type.
    explicit JSONValue(JSONValueType valueType, JSONNumberType numberType = JSONNT_NAN) :
        type_(0)
    {
        SetType(valueType, numberType);
    }

    /// Construct with a boolean.
    JSONValue(bool value) :         // NOLINT(google-explicit-constructor)
        type_(0)
    {
        *this = value;
    }
    /// Construct with a integer.
    JSONValue(int value) :          // NOLINT(google-explicit-constructor)
        type_(0)
    {
        *this = value;
    }
    /// Construct with a unsigned integer.
    JSONValue(unsigned value) :     // NOLINT(google-explicit-constructor)
        type_(0)
    {
        *this = value;
    }
    /// Construct with a float.
    JSONValue(float value) :        // NOLINT(google-explicit-constructor)
        type_(0)
    {
        *this = value;
    }
    /// Construct with a double.
    JSONValue(double value) :       // NOLINT(google-explicit-constructor)
        type_(0)
    {
        *this = value;
    }
    /// Construct with a string.
    JSONValue(const ea::string& value) :    // NOLINT(google-explicit-constructor)
        type_(0)
    {
        *this = value;
    }
    /// Construct with a C string.
    JSONValue(const char* value) :      // NOLINT(google-explicit-constructor)
        type_(0)
    {
        *this = value;
    }
    /// Construct with a JSON array.
    JSONValue(const ea::vector<JSONValue>& value) :     // NOLINT(google-explicit-constructor)
        type_(0)
    {
        *this = value;
    }
    /// Construct with a JSON object.
    JSONValue(const ea::map<ea::string, JSONValue>& value) :    // NOLINT(google-explicit-constructor)
        type_(0)
    {
        *this = value;
    }
    /// Copy-construct from another JSON value.
    JSONValue(const JSONValue& value) :
        type_(0)
    {
        *this = value;
    }
    /// Destruct.
    ~JSONValue()
    {
        SetType(JSON_NULL);
    }

    /// Assign from a boolean.
    JSONValue& operator =(bool rhs);
    /// Assign from an integer.
    JSONValue& operator =(int rhs);
    /// Assign from an unsigned integer.
    JSONValue& operator =(unsigned rhs);
    /// Assign from a float.
    JSONValue& operator =(float rhs);
    /// Assign from a double.
    JSONValue& operator =(double rhs);
    /// Assign from a string.
    JSONValue& operator =(const ea::string& rhs);
    /// Assign from a C string.
    JSONValue& operator =(const char* rhs);
    /// Assign from a JSON array.
    JSONValue& operator =(const ea::vector<JSONValue>& rhs);
    /// Assign from a JSON object.
    JSONValue& operator =(const ea::map<ea::string, JSONValue>& rhs);
    /// Assign from another JSON value.
    JSONValue& operator =(const JSONValue& rhs);
    /// Value equality operator.
    bool operator ==(const JSONValue& rhs) const;
    /// Value inequality operator.
    bool operator !=(const JSONValue& rhs) const;

    /// Return value type.
    JSONValueType GetValueType() const;
    /// Return number type.
    JSONNumberType GetNumberType() const;
    /// Return value type's name.
    ea::string GetValueTypeName() const;
    /// Return number type's name.
    ea::string GetNumberTypeName() const;

    /// Check is null.
    bool IsNull() const { return GetValueType() == JSON_NULL; }
    /// Check is boolean.
    bool IsBool() const { return GetValueType() == JSON_BOOL; }
    /// Check is number.
    bool IsNumber() const { return GetValueType() == JSON_NUMBER; }
    /// Check is string.
    bool IsString() const { return GetValueType() == JSON_STRING; }
    /// Check is array.
    bool IsArray() const { return GetValueType() == JSON_ARRAY; }
    /// Check is object.
    bool IsObject() const { return GetValueType() == JSON_OBJECT; }

    /// Return boolean value.
    bool GetBool() const { return IsBool() ? boolValue_ : false;}
    /// Return integer value.
    int GetInt() const { return IsNumber() ? (int)numberValue_ : 0; }
    /// Return unsigned integer value.
    unsigned GetUInt() const { return IsNumber() ? (unsigned)numberValue_ : 0; }
    /// Return float value.
    float GetFloat() const { return IsNumber() ? (float)numberValue_ : 0.0f; }
    /// Return double value.
    double GetDouble() const { return IsNumber() ? numberValue_ : 0.0; }
    /// Return string value.
    const ea::string& GetString() const { return IsString() ? *stringValue_ : EMPTY_STRING;}
    /// Return C string value.
    const char* GetCString() const { return IsString() ? stringValue_->c_str() : nullptr;}
    /// Return JSON array value.
    const ea::vector<JSONValue>& GetArray() const { return IsArray() ? *arrayValue_ : emptyArray; }
    /// Return JSON object value.
    const ea::map<ea::string, JSONValue>& GetObject() const { return IsObject() ? *objectValue_ : emptyObject; }

    // JSON array functions
    /// Return JSON value at index.
    JSONValue& operator [](unsigned index);
    /// Return JSON value at index.
    const JSONValue& operator [](unsigned index) const;
    /// Add JSON value at end.
    void Push(const JSONValue& value);
    /// Remove the last JSON value.
    void Pop();
    /// Insert an JSON value at position.
    void Insert(unsigned pos, const JSONValue& value);
    /// Erase a range of JSON values.
    void Erase(unsigned pos, unsigned length = 1);
    /// Resize array.
    void Resize(unsigned newSize);
    /// Return size of array or number of keys in object.
    unsigned Size() const;

    // JSON object functions
    /// Return JSON value with key.
    JSONValue& operator [](const ea::string& key);
    /// Return JSON value with key.
    const JSONValue& operator [](const ea::string& key) const;
    /// Set JSON value with key.
    void Set(const ea::string& key, const JSONValue& value);
    /// Return JSON value with key.
    const JSONValue& Get(const ea::string& key) const;
    /// Return JSON value with index.
    const JSONValue& Get(int index) const;
    /// Erase a pair by key.
    bool Erase(const ea::string& key);
    /// Return whether contains a pair with key.
    bool Contains(const ea::string& key) const;

    /// Clear array or object.
    void Clear();

    /// Set value type and number type, internal function.
    void SetType(JSONValueType valueType, JSONNumberType numberType = JSONNT_NAN);

    /// Set variant, context must provide for resource ref.
    void SetVariant(const Variant& variant, Context* context = nullptr);
    /// Return a variant.
    Variant GetVariant() const;
    /// Set variant value, context must provide for resource ref.
    void SetVariantValue(const Variant& variant, Context* context = nullptr);
    /// Return a variant with type.
    Variant GetVariantValue(VariantType type) const;
    /// Set variant map, context must provide for resource ref.
    void SetVariantMap(const VariantMap& variantMap, Context* context = nullptr);
    /// Return a variant map.
    VariantMap GetVariantMap() const;
    /// Set variant vector, context must provide for resource ref.
    void SetVariantVector(const VariantVector& variantVector, Context* context = nullptr);
    /// Return a variant vector.
    VariantVector GetVariantVector() const;

    /// Empty JSON value.
    static const JSONValue EMPTY;
    /// Empty JSON array.
    static const ea::vector<JSONValue> emptyArray;
    /// Empty JSON object.
    static const ea::map<ea::string, JSONValue> emptyObject;

    /// Return name corresponding to a value type.
    static ea::string GetValueTypeName(JSONValueType type);
    /// Return name corresponding to a number type.
    static ea::string GetNumberTypeName(JSONNumberType type);
    /// Return a value type from name; null if unrecognized.
    static JSONValueType GetValueTypeFromName(const ea::string& typeName);
    /// Return a value type from name; null if unrecognized.
    static JSONValueType GetValueTypeFromName(const char* typeName);
    /// Return a number type from name; NaN if unrecognized.
    static JSONNumberType GetNumberTypeFromName(const ea::string& typeName);
    /// Return a value type from name; NaN if unrecognized.
    static JSONNumberType GetNumberTypeFromName(const char* typeName);

//private:
    /// type.
    unsigned type_;
    union
    {
        /// Boolean value.
        bool boolValue_;
        /// Number value.
        double numberValue_;
        /// String value.
        ea::string* stringValue_;
        /// Array value.
        ea::vector<JSONValue>* arrayValue_;
        /// Object value.
        ea::map<ea::string, JSONValue>* objectValue_;
    };
};

/// Return iterator to the beginning.
URHO3D_API ea::map<ea::string, JSONValue>::iterator begin(JSONValue& value);
/// Return iterator to the beginning.
URHO3D_API ea::map<ea::string, JSONValue>::const_iterator begin(const JSONValue& value);
/// Return iterator to the end.
URHO3D_API ea::map<ea::string, JSONValue>::iterator end(JSONValue& value);
/// Return iterator to the beginning.
URHO3D_API ea::map<ea::string, JSONValue>::const_iterator end(const JSONValue& value);

/// JSON array type.
using JSONArray = ea::vector<JSONValue>;
/// JSON object type.
using JSONObject = ea::map<ea::string, JSONValue>;
/// JSON object iterator.
using JSONObjectIterator = ea::map<ea::string, JSONValue>::iterator;
/// Constant JSON object iterator.
using ConstJSONObjectIterator = ea::map<ea::string, JSONValue>::const_iterator;

}
