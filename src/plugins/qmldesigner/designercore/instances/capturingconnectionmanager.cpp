/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "capturingconnectionmanager.h"

#include <coreplugin/messagebox.h>

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

namespace QmlDesigner {

void CapturingConnectionManager::setUp(NodeInstanceServerProxy *nodeInstanceServerProxy,
                                       const QString &qrcMappingString,
                                       ProjectExplorer::Target *target)
{
    InteractiveConnectionManager::setUp(nodeInstanceServerProxy, qrcMappingString, target);

    int indexOfCapturePuppetStream = QCoreApplication::arguments().indexOf(
        "-capture-puppet-stream");
    if (indexOfCapturePuppetStream > 0) {
        m_captureFileForTest.setFileName(
            QCoreApplication::arguments().at(indexOfCapturePuppetStream + 1));
        bool isOpen = m_captureFileForTest.open(QIODevice::WriteOnly);
        qDebug() << "file is open: " << isOpen;
    }
}

void CapturingConnectionManager::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_captureFileForTest.isOpen()) {
        m_captureFileForTest.close();
        Core::AsynchronousMessageBox::warning(
            tr("QML Emulation Layer (QML Puppet) Crashed"),
            tr("You are recording a puppet stream and the emulations layer crashed. "
               "It is recommended to reopen the Qt Quick Designer and start again."));
    }

    InteractiveConnectionManager::processFinished(exitCode, exitStatus);
}

} // namespace QmlDesigner
