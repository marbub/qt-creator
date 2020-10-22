/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <qmetatype.h>
#include <QString>
#include <QDataStream>

#include "nodeinstanceglobal.h"

namespace QmlDesigner {

class InstanceContainer;

class InstanceContainer
{
    friend QDataStream &operator>>(QDataStream &in, InstanceContainer &container);

public:
    enum NodeSourceType { NoSource = 0, CustomParserSource = 1, ComponentSource = 2 };

    enum NodeMetaType { ObjectMetaType, ItemMetaType };

    enum NodeFlag { ParentTakesOverRendering = 1 };

    Q_DECLARE_FLAGS(NodeFlags, NodeFlag)

    InstanceContainer() = default;
    InstanceContainer(qint32 instanceId,
                      const TypeName &type,
                      int majorNumber,
                      int minorNumber,
                      const QString &componentPath,
                      const QString &nodeSource,
                      NodeSourceType nodeSourceType,
                      NodeMetaType metaType,
                      NodeFlags metaFlags)
        : instanceId(instanceId)
        , type(properDelemitingOfType(type))
        , majorNumber(majorNumber)
        , minorNumber(minorNumber)
        , componentPath(componentPath)
        , nodeSource(nodeSource)
        , nodeSourceType(nodeSourceType)
        , metaType(metaType)
        , metaFlags(metaFlags)
    {}

    static TypeName properDelemitingOfType(const TypeName &typeName)
    {
        TypeName convertedTypeName = typeName;
        int lastIndex = typeName.lastIndexOf('.');
        if (lastIndex > 0)
            convertedTypeName[lastIndex] = '/';

        return convertedTypeName;
    }

    friend QDataStream &operator<<(QDataStream &out, const InstanceContainer &container)
    {
        out << container.instanceId;
        out << container.type;
        out << container.majorNumber;
        out << container.minorNumber;
        out << container.componentPath;
        out << container.nodeSource;
        out << qint32(container.nodeSourceType);
        out << qint32(container.metaType);
        out << qint32(container.metaFlags);

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, InstanceContainer &container)
    {
        in >> container.instanceId;
        in >> container.type;
        in >> container.majorNumber;
        in >> container.minorNumber;
        in >> container.componentPath;
        in >> container.nodeSource;
        in >> container.nodeSourceType;
        in >> container.metaType;
        in >> container.metaFlags;

        return in;
    }

public:
    qint32 instanceId = -1;
    TypeName type;
    qint32 majorNumber = -1;
    qint32 minorNumber = -1;
    QString componentPath;
    QString nodeSource;
    NodeSourceType nodeSourceType = {};
    NodeMetaType metaType = {};
    NodeFlags metaFlags;
};

QDebug operator<<(QDebug debug, const InstanceContainer &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::InstanceContainer)
Q_DECLARE_OPERATORS_FOR_FLAGS(QmlDesigner::InstanceContainer::NodeFlags)
