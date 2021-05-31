/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "projectstorageids.h"

#include <utils/smallstring.h>

#include <vector>

namespace QmlDesigner::Storage {

enum class TypeAccessSemantics : int { Invalid, Reference, Value, Sequence, IsEnum = 1 << 8 };

enum class DeclarationTraits : unsigned int {
    Non = 0,
    IsReadOnly = 1 << 0,
    IsPointer = 1 << 1,
    IsList = 1 << 2
};

constexpr DeclarationTraits operator|(DeclarationTraits first, DeclarationTraits second)
{
    return static_cast<DeclarationTraits>(static_cast<int>(first) | static_cast<int>(second));
}

constexpr bool operator&(DeclarationTraits first, DeclarationTraits second)
{
    return static_cast<int>(first) & static_cast<int>(second);
}

class VersionNumber
{
public:
    explicit VersionNumber() = default;
    explicit VersionNumber(int version)
        : version{version}
    {}

    explicit operator bool() { return version >= 0; }

    friend bool operator==(VersionNumber first, VersionNumber second) noexcept
    {
        return first.version == second.version;
    }

public:
    int version = -1;
};

class Version
{
public:
    explicit Version() = default;
    explicit Version(VersionNumber major, VersionNumber minor = VersionNumber{})
        : major{major}
        , minor{minor}
    {}

    explicit Version(int major, int minor)
        : major{major}
        , minor{minor}
    {}

    explicit Version(int major)
        : major{major}
    {}

    friend bool operator==(Version first, Version second) noexcept
    {
        return first.major == second.major && first.minor == second.minor;
    }

    explicit operator bool() { return major && minor; }

public:
    VersionNumber major;
    VersionNumber minor;
};

class ExportedType
{
public:
    explicit ExportedType() = default;
    explicit ExportedType(Utils::SmallStringView qualifiedTypeName, Version version = Version{})
        : qualifiedTypeName{qualifiedTypeName}
        , version{version}
    {}

    explicit ExportedType(Utils::SmallStringView qualifiedTypeName, int majorVersion, int minorVersion)
        : qualifiedTypeName{qualifiedTypeName}
        , version{majorVersion, minorVersion}
    {}

public:
    Utils::SmallString qualifiedTypeName;
    Version version;
};

using ExportedTypes = std::vector<ExportedType>;

class EnumeratorDeclaration
{
public:
    explicit EnumeratorDeclaration() = default;
    explicit EnumeratorDeclaration(Utils::SmallStringView name, long long value, int hasValue = true)
        : name{name}
        , value{value}
        , hasValue{bool(hasValue)}
    {}

    explicit EnumeratorDeclaration(Utils::SmallStringView name)
        : name{name}
    {}

    friend bool operator==(const EnumeratorDeclaration &first, const EnumeratorDeclaration &second)
    {
        return first.name == second.name && first.value == second.value
               && first.hasValue == second.hasValue;
    }

public:
    Utils::SmallString name;
    long long value = 0;
    bool hasValue = false;
};

using EnumeratorDeclarations = std::vector<EnumeratorDeclaration>;

class EnumerationDeclaration
{
public:
    explicit EnumerationDeclaration() = default;
    explicit EnumerationDeclaration(Utils::SmallStringView name,
                                    EnumeratorDeclarations enumeratorDeclarations)
        : name{name}
        , enumeratorDeclarations{std::move(enumeratorDeclarations)}
    {}

    friend bool operator==(const EnumerationDeclaration &first, const EnumerationDeclaration &second)
    {
        return first.name == second.name
               && first.enumeratorDeclarations == second.enumeratorDeclarations;
    }

public:
    Utils::SmallString name;
    EnumeratorDeclarations enumeratorDeclarations;
};

using EnumerationDeclarations = std::vector<EnumerationDeclaration>;

class EnumerationDeclarationView
{
public:
    explicit EnumerationDeclarationView() = default;
    explicit EnumerationDeclarationView(Utils::SmallStringView name,
                                        Utils::SmallStringView enumeratorDeclarations,
                                        long long id)
        : name{name}
        , enumeratorDeclarations{std::move(enumeratorDeclarations)}
        , id{id}
    {}

public:
    Utils::SmallStringView name;
    Utils::SmallStringView enumeratorDeclarations;
    EnumerationDeclarationId id;
};

class ParameterDeclaration
{
public:
    explicit ParameterDeclaration() = default;
    explicit ParameterDeclaration(Utils::SmallStringView name,
                                  Utils::SmallStringView typeName,
                                  DeclarationTraits traits = {})
        : name{name}
        , typeName{typeName}
        , traits{traits}
    {}

    explicit ParameterDeclaration(Utils::SmallStringView name, Utils::SmallStringView typeName, int traits)
        : name{name}
        , typeName{typeName}
        , traits{static_cast<DeclarationTraits>(traits)}
    {}

    friend bool operator==(const ParameterDeclaration &first, const ParameterDeclaration &second)
    {
        return first.name == second.name && first.typeName == second.typeName
               && first.traits == second.traits;
    }

public:
    Utils::SmallString name;
    Utils::SmallString typeName;
    DeclarationTraits traits = {};
};

using ParameterDeclarations = std::vector<ParameterDeclaration>;

class SignalDeclaration
{
public:
    explicit SignalDeclaration() = default;
    explicit SignalDeclaration(Utils::SmallString name, ParameterDeclarations parameters)
        : name{name}
        , parameters{std::move(parameters)}
    {}

    explicit SignalDeclaration(Utils::SmallString name)
        : name{name}
    {}

    friend bool operator==(const SignalDeclaration &first, const SignalDeclaration &second)
    {
        return first.name == second.name && first.parameters == second.parameters;
    }

public:
    Utils::SmallString name;
    ParameterDeclarations parameters;
};

using SignalDeclarations = std::vector<SignalDeclaration>;

class SignalDeclarationView
{
public:
    explicit SignalDeclarationView() = default;
    explicit SignalDeclarationView(Utils::SmallStringView name,
                                   Utils::SmallStringView signature,
                                   long long id)
        : name{name}
        , signature{signature}
        , id{id}
    {}

public:
    Utils::SmallStringView name;
    Utils::SmallStringView signature;
    SignalDeclarationId id;
};

class FunctionDeclaration
{
public:
    explicit FunctionDeclaration() = default;
    explicit FunctionDeclaration(Utils::SmallStringView name,
                                 Utils::SmallStringView returnTypeName,
                                 ParameterDeclarations parameters)
        : name{name}
        , returnTypeName{returnTypeName}
        , parameters{std::move(parameters)}
    {}

    explicit FunctionDeclaration(Utils::SmallStringView name,
                                 Utils::SmallStringView returnTypeName = {})
        : name{name}
        , returnTypeName{returnTypeName}
    {}

    friend bool operator==(const FunctionDeclaration &first, const FunctionDeclaration &second)
    {
        return first.name == second.name && first.returnTypeName == second.returnTypeName
               && first.parameters == second.parameters;
    }

public:
    Utils::SmallString name;
    Utils::SmallString returnTypeName;
    ParameterDeclarations parameters;
};

using FunctionDeclarations = std::vector<FunctionDeclaration>;

class FunctionDeclarationView
{
public:
    explicit FunctionDeclarationView() = default;
    explicit FunctionDeclarationView(Utils::SmallStringView name,
                                     Utils::SmallStringView returnTypeName,
                                     Utils::SmallStringView signature,
                                     long long id)
        : name{name}
        , returnTypeName{returnTypeName}
        , signature{signature}
        , id{id}
    {}

public:
    Utils::SmallStringView name;
    Utils::SmallStringView returnTypeName;
    Utils::SmallStringView signature;
    FunctionDeclarationId id;
};

class PropertyDeclaration
{
public:
    explicit PropertyDeclaration() = default;
    explicit PropertyDeclaration(Utils::SmallStringView name,
                                 Utils::SmallStringView typeName,
                                 DeclarationTraits traits)
        : name{name}
        , typeName{typeName}
        , traits{traits}
    {}

    explicit PropertyDeclaration(Utils::SmallStringView name, Utils::SmallStringView typeName, int traits)
        : name{name}
        , typeName{typeName}
        , traits{static_cast<DeclarationTraits>(traits)}
    {}

public:
    Utils::SmallString name;
    Utils::SmallString typeName;
    DeclarationTraits traits = {};
    TypeId typeId;
};

using PropertyDeclarations = std::vector<PropertyDeclaration>;

class PropertyDeclarationView
{
public:
    explicit PropertyDeclarationView(Utils::SmallStringView name,
                                     int traits,
                                     long long typeId,
                                     long long id)
        : name{name}
        , traits{static_cast<DeclarationTraits>(traits)}
        , typeId{typeId}
        , id{id}

    {}

public:
    Utils::SmallStringView name;
    DeclarationTraits traits = {};
    TypeId typeId;
    PropertyDeclarationId id;
};

class Type
{
public:
    explicit Type() = default;
    explicit Type(Utils::SmallStringView typeName,
                  Utils::SmallStringView prototype,
                  TypeAccessSemantics accessSemantics,
                  SourceId sourceId,
                  ExportedTypes exportedTypes = {},
                  PropertyDeclarations propertyDeclarations = {},
                  FunctionDeclarations functionDeclarations = {},
                  SignalDeclarations signalDeclarations = {},
                  EnumerationDeclarations enumerationDeclarations = {},
                  TypeId typeId = TypeId{})
        : typeName{typeName}
        , prototype{prototype}
        , exportedTypes{std::move(exportedTypes)}
        , propertyDeclarations{std::move(propertyDeclarations)}
        , functionDeclarations{std::move(functionDeclarations)}
        , signalDeclarations{std::move(signalDeclarations)}
        , enumerationDeclarations{std::move(enumerationDeclarations)}
        , accessSemantics{accessSemantics}
        , sourceId{sourceId}
        , typeId{typeId}
    {}

    explicit Type(Utils::SmallStringView typeName,
                  Utils::SmallStringView prototype,
                  int accessSemantics,
                  int sourceId)
        : typeName{typeName}
        , prototype{prototype}
        , accessSemantics{static_cast<TypeAccessSemantics>(accessSemantics)}
        , sourceId{sourceId}
    {}

    explicit Type(Utils::SmallStringView typeName,
                  long long typeId,
                  Utils::SmallStringView prototype,
                  int accessSemantics,
                  int sourceId)
        : typeName{typeName}
        , prototype{prototype}
        , accessSemantics{static_cast<TypeAccessSemantics>(accessSemantics)}
        , sourceId{sourceId}
        , typeId{typeId}
    {}

public:
    Utils::SmallString typeName;
    Utils::SmallString prototype;
    Utils::SmallString attachedType;
    ExportedTypes exportedTypes;
    PropertyDeclarations propertyDeclarations;
    FunctionDeclarations functionDeclarations;
    SignalDeclarations signalDeclarations;
    EnumerationDeclarations enumerationDeclarations;
    TypeAccessSemantics accessSemantics = TypeAccessSemantics::Invalid;
    SourceId sourceId;
    TypeId typeId;
    bool isCreatable = false;
};

using Types = std::vector<Type>;

} // namespace QmlDesigner::Storage
