//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include "../Container/Ptr.h"
#include "../Core/Variant.h"
#include "../Math/BoundingBox.h"
#include "../Math/Rect.h"

#include <EASTL/unique_ptr.h>

namespace pugi
{

struct xml_node_struct;
class xpath_node;
class xpath_node_set;
class xpath_query;
class xpath_variable_set;

}

namespace Urho3D
{

class Context;
class XMLFile;
class XPathQuery;
class XPathResultSet;

/// Element in an XML file.
class URHO3D_API XMLElement
{
public:
    /// Construct null element.
    XMLElement();
    /// Construct with document and node pointers.
    XMLElement(XMLFile* file, pugi::xml_node_struct* node);
    /// Construct from xpath query result set.
    XMLElement(XMLFile* file, const XPathResultSet* resultSet, const pugi::xpath_node* xpathNode, unsigned xpathResultIndex);
    /// Copy-construct from another element.
    XMLElement(const XMLElement& rhs);
    /// Destruct.
    ~XMLElement();
    /// Assignment operator.
    XMLElement& operator =(const XMLElement& rhs);

    /// Set element name.
    void SetName(const ea::string& name);
    /// Set element name.
    void SetName(const char* name);
    /// Create a child element.
    XMLElement CreateChild(const ea::string& name);
    /// Create a child element.
    XMLElement CreateChild(const char* name);
    /// Return the first child element with name or create if does not exist.
    XMLElement GetOrCreateChild(const ea::string& name);
    /// Return the first child element with name or create if does not exist.
    XMLElement GetOrCreateChild(const char* name);
    /// Append element. If asCopy is set to true then original element is copied and appended, otherwise specified element is appended.
    bool AppendChild(XMLElement element, bool asCopy = false);
    /// Remove element from its parent.
    bool Remove();
    /// Remove a child element. Return true if successful.
    bool RemoveChild(const XMLElement& element);
    /// Remove a child element by name. Return true if successful.
    bool RemoveChild(const ea::string& name);
    /// Remove a child element by name. Return true if successful.
    bool RemoveChild(const char* name);
    /// Remove child elements of certain name, or all child elements if name is empty. Return true if successful.
    bool RemoveChildren(const ea::string& name = EMPTY_STRING);
    /// Remove child elements of certain name, or all child elements if name is empty. Return true if successful.
    bool RemoveChildren(const char* name);
    /// Remove an attribute by name. Return true if successful.
    bool RemoveAttribute(const ea::string& name = EMPTY_STRING);
    /// Remove an attribute by name. Return true if successful.
    bool RemoveAttribute(const char* name);

    /// Select an element/attribute using XPath query.
    XMLElement SelectSingle(const ea::string& query, pugi::xpath_variable_set* variables = nullptr) const;
    /// Select an element/attribute using XPath query.
    XMLElement SelectSinglePrepared(const XPathQuery& query) const;
    /// Select elements/attributes using XPath query.
    XPathResultSet Select(const ea::string& query, pugi::xpath_variable_set* variables = nullptr) const;
    /// Select elements/attributes using XPath query.
    XPathResultSet SelectPrepared(const XPathQuery& query) const;

    /// Set the value for an inner node in the following format <node>value</node>.
    /// @property
    bool SetValue(const ea::string& value);
    /// Set the value for an inner node in the following format <node>value</node>. Must be used on the <node> element.
    bool SetValue(const char* value);
    /// Set an attribute.
    bool SetAttribute(const ea::string& name, const ea::string& value);
    /// Set an attribute.
    bool SetAttribute(const char* name, const char* value);
    /// Set an attribute. Only valid if it is an attribute only XPath query result.
    bool SetAttribute(const ea::string& value);
    /// Set an attribute. Only valid if it is an attribute only XPath query result.
    bool SetAttribute(const char* value);
    /// Set a bool attribute.
    bool SetBool(const ea::string& name, bool value);
    /// Set a BoundingBox attribute.
    bool SetBoundingBox(const BoundingBox& value);
    /// Set a buffer attribute.
    bool SetBuffer(const ea::string& name, const void* data, unsigned size);
    /// Set a buffer attribute.
    bool SetBuffer(const ea::string& name, const ea::vector<unsigned char>& value);
    /// Set a color attribute.
    bool SetColor(const ea::string& name, const Color& value);
    /// Set a float attribute.
    bool SetFloat(const ea::string& name, float value);
    /// Set a double attribute.
    bool SetDouble(const ea::string& name, double value);
    /// Set an unsigned integer attribute.
    bool SetUInt(const ea::string& name, unsigned value);
    /// Set an integer attribute.
    bool SetInt(const ea::string& name, int value);
    /// Set an unsigned long long integer attribute.
    bool SetUInt64(const ea::string& name, unsigned long long value);
    /// Set a long long integer attribute.
    bool SetInt64(const ea::string& name, long long value);
    /// Set an IntRect attribute.
    bool SetIntRect(const ea::string& name, const IntRect& value);
    /// Set an IntVector2 attribute.
    bool SetIntVector2(const ea::string& name, const IntVector2& value);
    /// Set an IntVector3 attribute.
    bool SetIntVector3(const ea::string& name, const IntVector3& value);
    /// Set a Rect attribute.
    bool SetRect(const ea::string& name, const Rect& value);
    /// Set a quaternion attribute.
    bool SetQuaternion(const ea::string& name, const Quaternion& value);
    /// Set a string attribute.
    bool SetString(const ea::string& name, const ea::string& value);
    /// Set a variant attribute.
    bool SetVariant(const Variant& value);
    /// Set a variant attribute excluding the type.
    bool SetVariantValue(const Variant& value);
    /// Set a resource reference attribute.
    bool SetResourceRef(const ResourceRef& value);
    /// Set a resource reference list attribute.
    bool SetResourceRefList(const ResourceRefList& value);
    /// Set a variant vector attribute. Creates child elements as necessary.
    bool SetVariantVector(const VariantVector& value);
    /// Set a string vector attribute. Creates child elements as necessary.
    bool SetStringVector(const StringVector& value);
    /// Set a variant map attribute. Creates child elements as necessary.
    bool SetVariantMap(const VariantMap& value);
    /// Set a Vector2 attribute.
    bool SetVector2(const ea::string& name, const Vector2& value);
    /// Set a Vector3 attribute.
    bool SetVector3(const ea::string& name, const Vector3& value);
    /// Set a Vector4 attribute.
    bool SetVector4(const ea::string& name, const Vector4& value);
    /// Set a float, Vector or Matrix attribute stored in a variant.
    bool SetVectorVariant(const ea::string& name, const Variant& value);
    /// Set a Matrix3 attribute.
    bool SetMatrix3(const ea::string& name, const Matrix3& value);
    /// Set a Matrix3x4 attribute.
    bool SetMatrix3x4(const ea::string& name, const Matrix3x4& value);
    /// Set a Matrix4 attribute.
    bool SetMatrix4(const ea::string& name, const Matrix4& value);

    /// Return whether does not refer to an element or an XPath node.
    /// @property{get_isNull}
    bool IsNull() const;
    /// Return whether refers to an element or an XPath node.
    /// @property
    bool NotNull() const;
    /// Return true if refers to an element or an XPath node.
    explicit operator bool() const;
    /// Return element name (or attribute name if it is an attribute only XPath query result).
    /// @property
    ea::string GetName() const;
    /// Return whether has a child element.
    bool HasChild(const ea::string& name) const;
    /// Return whether has a child element.
    bool HasChild(const char* name) const;
    /// Return child element, or null if missing.
    XMLElement GetChild(const ea::string& name = EMPTY_STRING) const;
    /// Return child element, or null if missing.
    XMLElement GetChild(const char* name) const;
    /// Return next sibling element.
    XMLElement GetNext(const ea::string& name = EMPTY_STRING) const;
    /// Return next sibling element.
    XMLElement GetNext(const char* name) const;
    /// Return parent element.
    /// @property
    XMLElement GetParent() const;
    /// Return number of attributes.
    /// @property
    unsigned GetNumAttributes() const;
    /// Return whether has an attribute.
    bool HasAttribute(const ea::string& name) const;
    /// Return whether has an attribute.
    bool HasAttribute(const char* name) const;
    /// Return inner value, or empty if missing for nodes like <node>value</node>.
    /// @property
    ea::string GetValue() const;
    /// Return attribute, or empty if missing.
    ea::string GetAttribute(const ea::string& name = EMPTY_STRING) const;
    /// Return attribute, or empty if missing.
    ea::string GetAttribute(const char* name) const;
    /// Return attribute as C string, or null if missing.
    const char* GetAttributeCString(const char* name) const;
    /// Return attribute in lowercase, or empty if missing.
    ea::string GetAttributeLower(const ea::string& name) const;
    /// Return attribute in lowercase, or empty if missing.
    ea::string GetAttributeLower(const char* name) const;
    /// Return attribute in lowercase, or empty if missing.
    ea::string GetAttributeUpper(const ea::string& name) const;
    /// Return attribute in lowercase, or empty if missing.
    ea::string GetAttributeUpper(const char* name) const;
    /// Return names of all attributes.
    ea::vector<ea::string> GetAttributeNames() const;
    /// Return bool attribute, or false if missing.
    bool GetBool(const ea::string& name) const;
    /// Return buffer attribute, or empty if missing.
    ea::vector<unsigned char> GetBuffer(const ea::string& name) const;
    /// Copy buffer attribute into a supplied buffer. Return true if buffer was large enough.
    bool GetBuffer(const ea::string& name, void* dest, unsigned size) const;
    /// Return bounding box attribute, or empty if missing.
    BoundingBox GetBoundingBox() const;
    /// Return a color attribute, or default if missing.
    Color GetColor(const ea::string& name) const;
    /// Return a float attribute, or zero if missing.
    float GetFloat(const ea::string& name) const;
    /// Return a double attribute, or zero if missing.
    double GetDouble(const ea::string& name) const;
    /// Return an unsigned integer attribute, or zero if missing.
    unsigned GetUInt(const ea::string& name) const;
    /// Return an integer attribute, or zero if missing.
    int GetInt(const ea::string& name) const;
    /// Return an unsigned long long integer attribute, or zero if missing.
    unsigned long long GetUInt64(const ea::string& name) const;
    /// Return a long long integer attribute, or zero if missing.
    long long GetInt64(const ea::string& name) const;
    /// Return an IntRect attribute, or default if missing.
    IntRect GetIntRect(const ea::string& name) const;
    /// Return an IntVector2 attribute, or default if missing.
    IntVector2 GetIntVector2(const ea::string& name) const;
    /// Return an IntVector3 attribute, or default if missing.
    IntVector3 GetIntVector3(const ea::string& name) const;
    /// Return a Rect attribute, or default if missing.
    Rect GetRect(const ea::string& name) const;
    /// Return a quaternion attribute, or default if missing.
    Quaternion GetQuaternion(const ea::string& name) const;
    /// Return a variant attribute, or empty if missing.
    Variant GetVariant() const;
    /// Return a variant attribute with static type. Context must be provided for loading SharedPtr<Serializable>.
    Variant GetVariantValue(VariantType type, Context* context=nullptr) const;
    /// Return a resource reference attribute, or empty if missing.
    ResourceRef GetResourceRef() const;
    /// Return a resource reference list attribute, or empty if missing.
    ResourceRefList GetResourceRefList() const;
    /// Return a variant vector attribute, or empty if missing.
    VariantVector GetVariantVector() const;
    /// Return a string vector attribute, or empty if missing.
    StringVector GetStringVector() const;
    /// Return a variant map attribute, or empty if missing.
    VariantMap GetVariantMap() const;
    /// Return a Vector2 attribute, or zero vector if missing.
    Vector2 GetVector2(const ea::string& name) const;
    /// Return a Vector3 attribute, or zero vector if missing.
    Vector3 GetVector3(const ea::string& name) const;
    /// Return a Vector4 attribute, or zero vector if missing.
    Vector4 GetVector4(const ea::string& name) const;
    /// Return any Vector attribute as Vector4. Missing coordinates will be zero.
    Vector4 GetVector(const ea::string& name) const;
    /// Return a float, Vector or Matrix attribute as Variant.
    Variant GetVectorVariant(const ea::string& name) const;
    /// Return a Matrix3 attribute, or zero matrix if missing.
    Matrix3 GetMatrix3(const ea::string& name) const;
    /// Return a Matrix3x4 attribute, or zero matrix if missing.
    Matrix3x4 GetMatrix3x4(const ea::string& name) const;
    /// Return a Matrix4 attribute, or zero matrix if missing.
    Matrix4 GetMatrix4(const ea::string& name) const;
    /// Return XML file.
    /// @property
    XMLFile* GetFile() const;

    /// Return pugixml xml_node_struct.
    pugi::xml_node_struct* GetNode() const { return node_; }

    /// Return XPath query result set.
    const XPathResultSet* GetXPathResultSet() const { return xpathResultSet_; }

    /// Return pugixml xpath_node.
    const pugi::xpath_node* GetXPathNode() const { return xpathNode_; }

    /// Return current result index.
    unsigned GetXPathResultIndex() const { return xpathResultIndex_; }

    /// Return next XPath query result. Only valid when this instance of XMLElement is itself one of the query result in the result set.
    /// @property
    XMLElement NextResult() const;

    /// Empty XMLElement.
    static const XMLElement EMPTY;

private:
    /// XML file.
    WeakPtr<XMLFile> file_;
    /// Pugixml node.
    pugi::xml_node_struct* node_;
    /// XPath query result set.
    const XPathResultSet* xpathResultSet_;
    /// Pugixml xpath_node.
    const pugi::xpath_node* xpathNode_;
    /// Current XPath query result index (used internally to advance to subsequent query result).
    mutable unsigned xpathResultIndex_;
};

/// XPath query result set.
class URHO3D_API XPathResultSet
{
public:
    /// Construct empty result set.
    XPathResultSet();
    /// Construct with result set from XPath query.
    XPathResultSet(XMLFile* file, pugi::xpath_node_set* resultSet);
    /// Copy-construct.
    XPathResultSet(const XPathResultSet& rhs);
    /// Destruct.
    ~XPathResultSet();
    /// Assignment operator.
    XPathResultSet& operator =(const XPathResultSet& rhs);
    /// Return the n-th result in the set. Call XMLElement::GetNextResult() to get the subsequent result in the set.
    /// Note: The XPathResultSet return value must be stored in a lhs variable to ensure the underlying xpath_node_set* is still valid while performing XPathResultSet::FirstResult(), XPathResultSet::operator [], and XMLElement::NextResult().
    XMLElement operator [](unsigned index) const;
    /// Return the first result in the set. Call XMLElement::GetNextResult() to get the subsequent result in the set.
    /// Note: The XPathResultSet return value must be stored in a lhs variable to ensure the underlying xpath_node_set* is still valid while performing XPathResultSet::FirstResult(), XPathResultSet::operator [], and XMLElement::NextResult().
    /// @property
    XMLElement FirstResult();
    /// Return size of result set.
    /// @property
    unsigned Size() const;
    /// Return whether result set is empty.
    /// @property
    bool Empty() const;

    /// Return pugixml xpath_node_set.
    pugi::xpath_node_set* GetXPathNodeSet() const { return resultSet_; }

private:
    /// XML file.
    WeakPtr<XMLFile> file_;
    /// Pugixml xpath_node_set.
    pugi::xpath_node_set* resultSet_;
};

/// XPath query.
class URHO3D_API XPathQuery
{
public:
    /// Construct empty.
    XPathQuery();
    /// Construct XPath query object with query string and variable string. The variable string format is "name1:type1,name2:type2,..." where type is one of "Bool", "Float", "String", "ResultSet".
    explicit XPathQuery(const ea::string& queryString, const ea::string& variableString = EMPTY_STRING);
    /// Destruct.
    ~XPathQuery();
    /// Bind query object with variable set.
    void Bind();
    /// Add/Set a bool variable. Return true if successful.
    bool SetVariable(const ea::string& name, bool value);
    /// Add/Set a float variable. Return true if successful.
    bool SetVariable(const ea::string& name, float value);
    /// Add/Set a string variable. Return true if successful.
    bool SetVariable(const ea::string& name, const ea::string& value);
    /// Add/Set a string variable. Return true if successful.
    bool SetVariable(const char* name, const char* value);
    /// Add/Set an XPath query result set variable. Return true if successful.
    bool SetVariable(const ea::string& name, const XPathResultSet& value);
    /// Set XPath query string and variable string. The variable string format is "name1:type1,name2:type2,..." where type is one of "Bool", "Float", "String", "ResultSet".
    bool SetQuery(const ea::string& queryString, const ea::string& variableString = EMPTY_STRING, bool bind = true);
    /// Clear by removing all variables and XPath query object.
    void Clear();
    /// Evaluate XPath query and expecting a boolean return value.
    bool EvaluateToBool(const XMLElement& element) const;
    /// Evaluate XPath query and expecting a float return value.
    float EvaluateToFloat(const XMLElement& element) const;
    /// Evaluate XPath query and expecting a string return value.
    ea::string EvaluateToString(const XMLElement& element) const;
    /// Evaluate XPath query and expecting an XPath query result set as return value.
    /// Note: The XPathResultSet return value must be stored in a lhs variable to ensure the underlying xpath_node_set* is still valid while performing XPathResultSet::FirstResult(), XPathResultSet::operator [], and XMLElement::NextResult().
    XPathResultSet Evaluate(const XMLElement& element) const;

    /// Return query string.
    /// @property
    ea::string GetQuery() const { return queryString_; }

    /// Return pugixml xpath_query.
    pugi::xpath_query* GetXPathQuery() const { return query_.get(); }

    /// Return pugixml xpath_variable_set.
    pugi::xpath_variable_set* GetXPathVariableSet() const { return variables_.get(); }

private:
    /// XPath query string.
    ea::string queryString_;
    /// Pugixml xpath_query.
    ea::unique_ptr<pugi::xpath_query> query_;
    /// Pugixml xpath_variable_set.
    ea::unique_ptr<pugi::xpath_variable_set> variables_;
};

}
