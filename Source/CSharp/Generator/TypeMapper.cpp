//
// Copyright (c) 2008-2018 the Urho3D project.
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

#include <Urho3D/Resource/XMLElement.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Printer/CodePrinter.h>
#include "TypeMapper.h"
#include "Utilities.h"
#include "GeneratorContext.h"


namespace Urho3D
{

TypeMapper::TypeMapper(Context* context)
    : Object(context)
{
}

void TypeMapper::Load(XMLFile* rules)
{
    XMLElement typeMaps = rules->GetRoot().GetChild("typemaps");
    for (auto typeMap = typeMaps.GetChild("typemap"); typeMap.NotNull(); typeMap = typeMap.GetNext("typemap"))
    {
        TypeMap map;
        map.cppType_ = typeMap.GetAttribute("type");
        map.cType_ = typeMap.GetAttribute("ctype");
        map.csType_ = typeMap.GetAttribute("cstype");
        map.pInvokeType_ = typeMap.GetAttribute("ptype");
        map.pInvokeValueType_ = typeMap.GetAttribute("pvaltype");

        if (map.cType_.Empty())
            map.cType_ = map.cppType_;

        if (map.pInvokeType_.Empty())
            map.pInvokeType_ = ToPInvokeType(map.cType_, "");

        if (map.csType_.Empty())
            map.csType_ = map.pInvokeType_;

        if (map.pInvokeValueType_.Empty())
            map.pInvokeValueType_ = map.pInvokeType_;

        if (auto cppToC = typeMap.GetChild("cpp_to_c"))
            map.cppToCTemplate_ = cppToC.GetValue();

        if (auto cToCpp = typeMap.GetChild("c_to_cpp"))
            map.cToCppTemplate_ = cToCpp.GetValue();

        if (auto cppToCValue = typeMap.GetChild("cpp_to_c_value"))
            map.cppToCValueTemplate_ = cppToCValue.GetValue();

        if (map.cppToCValueTemplate_.Empty())
            map.cppToCValueTemplate_ = map.cppToCTemplate_;

        if (auto toCS = typeMap.GetChild("pinvoke_to_cs"))
            map.pInvokeToCSTemplate_ = toCS.GetValue();

        if (auto copyToPInvoke = typeMap.GetChild("pinvoke_to_cs_value"))
            map.pInvokeToCSValueTemplate_ = copyToPInvoke.GetValue();

        if (map.pInvokeToCSValueTemplate_.Empty())
            map.pInvokeToCSValueTemplate_ = map.pInvokeToCSTemplate_;

        if (auto toPInvoke = typeMap.GetChild("cs_to_pinvoke"))
            map.csToPInvokeTemplate_ = toPInvoke.GetValue();

        typeMaps_[map.cppType_] = map;
    }
}

const TypeMap* TypeMapper::GetTypeMap(const cppast::cpp_type& type)
{
    String baseName = Urho3D::GetTypeName(type);
    String fullName = cppast::to_string(type);

    auto it = typeMaps_.Find(baseName);
    if (it == typeMaps_.End())
        it = typeMaps_.Find(fullName);

    if (it != typeMaps_.End())
        return &it->second_;

    return nullptr;
}

const TypeMap* TypeMapper::GetTypeMap(const String& typeName)
{
    auto it = typeMaps_.Find(typeName);
    if (it != typeMaps_.End())
        return &it->second_;

    return nullptr;
}

String TypeMapper::ToCType(const cppast::cpp_type& type)
{
    const auto* map = GetTypeMap(type);

    if (map)
        return map->cType_;

    String typeName = cppast::to_string(type);
    if (type.kind() == cppast::cpp_type_kind::builtin_t)
        return typeName;
    else if (type.kind() != cppast::cpp_type_kind::pointer_t && type.kind() != cppast::cpp_type_kind::reference_t)
        // A value type is turned into pointer.
        return typeName + "*";

    return typeName;
}

String TypeMapper::ToPInvokeType(const cppast::cpp_type& type, const String& default_)
{
    if (const auto* map = GetTypeMap(type))
    {
        if (IsComplexValueType(type))
            return map->pInvokeValueType_;
        else
            return map->pInvokeType_;
    }
    else
    {
        String name = cppast::to_string(type);
        String result = ToPInvokeType(Urho3D::GetTypeName(type), ToPInvokeType(name));
        if (result.Empty())
            result = default_;
        return result;
    }
}

String TypeMapper::ToPInvokeType(const String& name, const String& default_)
{
    if (name == "char const*")
        return "string";
    if (name == "void*")
        return "IntPtr";
    if (name == "char")
        return "char";
    if (name == "unsigned char")
        return "byte";
    if (name == "short")
        return "short";
    if (name == "unsigned short")
        return "ushort";
    if (name == "int")
        return "int";
    if (name == "unsigned int" || name == "unsigned")
        return "uint";
    if (name == "long long")
        return "long";
    if (name == "unsigned long long")
        return "ulong";
    if (name == "void")
        return "void";
    if (name == "bool")
        return "bool";
    if (name == "float")
        return "float";
    if (name == "double")
        return "double";
    if (name == "char const*")
        return "string";

    return default_;
}

String TypeMapper::ToPInvokeTypeReturn(const cppast::cpp_type& type, bool canCopy)
{
    String result = ToPInvokeType(type);
    return result;
}

String TypeMapper::ToPInvokeTypeParam(const cppast::cpp_type& type)
{
    String result = ToPInvokeType(type);
    if (result == "string")
        return "[param: MarshalAs(UnmanagedType.LPUTF8Str)]" + result;
    return result;
}

String TypeMapper::MapToC(const cppast::cpp_type& type, const String& expression, bool canCopy)
{
    const auto* map = GetTypeMap(type);
    String result = expression;

    if (map)
    {
        if (IsComplexValueType(type))
            result = fmt(map->cppToCValueTemplate_.CString(), {{"value", result.CString()}});
        else
            result = fmt(map->cppToCTemplate_.CString(), {{"value", result.CString()}});
    }
    else if (IsComplexValueType(type))
        // A unmapped value type - return it's address.
        result = "&" + result;

    return result;
}

String TypeMapper::MapToCpp(const cppast::cpp_type& type, const String& expression)
{
    const auto* map = GetTypeMap(type);
    String result = expression;

    if (map)
        result = fmt(map->cToCppTemplate_.CString(), {{"value", result.CString()}});
    else if (IsComplexValueType(type))
        // A unmapped value type - dereference.
        result = "*" + result;

    return result;
}

String TypeMapper::ToCSType(const cppast::cpp_type& type)
{
    String result;
    if (const auto* map = GetTypeMap(type))
        result = map->csType_;
    else if (GetSubsystem<GeneratorContext>()->IsKnownType(type))
        return Urho3D::GetTypeName(type).Replaced("::", ".");
    else
        result = ToPInvokeType(type);
    return result;
}

String TypeMapper::MapToPInvoke(const cppast::cpp_type& type, const String& expression)
{
    if (const auto* map = GetTypeMap(type))
        return fmt(map->csToPInvokeTemplate_.CString(), {{"value", expression.CString()}});
    else if (GetSubsystem<GeneratorContext>()->IsKnownType(type))
        return expression + ".instance_";
    return expression;
}

String TypeMapper::MapToCS(const cppast::cpp_type& type, const String& expression, bool canCopy)
{
    String result = expression;
    if (const auto* map = GetTypeMap(type))
    {
        if (IsComplexValueType(type))
            result = fmt(map->pInvokeToCSValueTemplate_.CString(), {{"value", result.CString()}});
        else
            result = fmt(map->pInvokeToCSTemplate_.CString(), {{"value", result.CString()}});
    }
    else if (GetSubsystem<GeneratorContext>()->IsKnownType(type))
    {
        // Class references are cached
        String returnType = Urho3D::GetTypeName(type).Replaced("::", ".");
        result = fmt("{{return_type}}.cache_.GetOrAdd({{call}}, (instance) => { return new {{return_type}}(instance); })",
                   {{"call",        result.CString()},
                    {"return_type", returnType.CString()}});
    }
    return result;
}

}
