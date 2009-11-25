/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QT4BUILDCONFIGURATION_H
#define QT4BUILDCONFIGURATION_H

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/toolchain.h>

namespace Qt4ProjectManager {

class Qt4Project;
class QtVersion;
class QMakeStep;
class MakeStep;

namespace Internal {

class Qt4BuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
public:
    Qt4BuildConfiguration(Qt4Project *pro);
    // copy ctor
    Qt4BuildConfiguration(Qt4BuildConfiguration *source);
    ~Qt4BuildConfiguration();

    ProjectExplorer::Environment environment() const;
    ProjectExplorer::Environment baseEnvironment() const;
    void setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff);
    QList<ProjectExplorer::EnvironmentItem> userEnvironmentChanges() const;
    bool useSystemEnvironment() const;
    void setUseSystemEnvironment(bool b);

    virtual QString buildDirectory() const;

    // returns the qtdir (depends on the current QtVersion)
    QString qtDir() const;
    //returns the qtVersion, if the project is set to use the default qt version, then
    // that is returned
    // to check wheter the project uses the default qt version use qtVersionId
    QtVersion *qtVersion() const;

    // returns the id of the qt version, if the project is using the default qt version
    // this function returns 0
    int qtVersionId() const;
    //returns the name of the qt version, might be QString::Null, which means default qt version
    // qtVersion is in general the better method to use
    QString qtVersionName() const;

    void setQtVersion(int id);

    ProjectExplorer::ToolChain *toolChain() const;
    void setToolChainType(ProjectExplorer::ToolChain::ToolChainType type);
    ProjectExplorer::ToolChain::ToolChainType toolChainType() const;


    // Those functions are used in a few places.
    // The drawback is that we shouldn't actually depend on them beeing always there
    // That is generally the stuff that is asked should normally be transfered to
    // Qt4Project *
    // So that we can later enable people to build qt4projects the way they would like
    QMakeStep *qmakeStep() const;
    MakeStep *makeStep() const;

    QString makeCommand() const;
    QString defaultMakeTarget() const;

    // TODO rename
    bool compareBuildConfigurationToImportFrom(const QString &workingDirectory);

signals:
    void qtVersionChanged();
};

} // namespace Qt4ProjectManager
} // namespace Internal

#endif // QT4BUILDCONFIGURATION_H
