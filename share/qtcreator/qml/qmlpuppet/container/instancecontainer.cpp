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

#include "instancecontainer.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

QDebug operator <<(QDebug debug, const InstanceContainer &command)
{
    debug.nospace() << "InstanceContainer("
                    << "instanceId: " << command.instanceId << ", "
                    << "type: " << command.type << ", "
                    << "majorNumber: " << command.majorNumber << ", "
                    << "minorNumber: " << command.minorNumber << ", ";

    if (!command.componentPath.isEmpty())
        debug.nospace() << "componentPath: " << command.componentPath << ", ";

    if (!command.nodeSource.isEmpty())
        debug.nospace() << "nodeSource: " << command.nodeSource << ", ";

    if (command.nodeSourceType == InstanceContainer::NoSource)
        debug.nospace() << "nodeSourceType: NoSource, ";
    else if (command.nodeSourceType == InstanceContainer::CustomParserSource)
        debug.nospace() << "nodeSourceType: CustomParserSource, ";
    else
        debug.nospace() << "nodeSourceType: ComponentSource, ";

    if (command.metaType == InstanceContainer::ObjectMetaType)
        debug.nospace() << "metatype: ObjectMetaType";
    else
        debug.nospace() << "metatype: ItemMetaType";

    return debug.nospace() << ")";

}

} // namespace QmlDesigner
