/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <vector>

namespace QmlDesigner {

template<auto Type>
class InternalId
{
public:
    explicit InternalId() = default;
    explicit InternalId(long long id)
        : id(id)
    {}

    friend bool operator==(InternalId first, InternalId second)
    {
        return first.id == second.id && first.isValid() && second.isValid();
    }

    friend bool operator!=(InternalId first, InternalId second) { return !(first == second); }

    friend bool operator<(InternalId first, InternalId second) { return first.id < second.id; }

    bool isValid() const { return id >= 0; }

public:
    long long id = -1;
};

enum InternalIdType { Type, PropertyType, PropertyDeclaration };

using InternalTypeId = InternalId<InternalIdType::Type>;
using InternalTypeIds = std::vector<InternalTypeId>;

using InternalPropertyDeclarationId = InternalId<InternalIdType::PropertyDeclaration>;
using InternalPropertyDeclarationIds = std::vector<InternalPropertyDeclarationId>;

enum class TypeAccessSemantics { Reference, Value, Sequence, IsEnum = 0xF };
} // namespace QmlDesigner
