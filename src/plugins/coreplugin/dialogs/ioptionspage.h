/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef IOPTIONSPAGE_H
#define IOPTIONSPAGE_H

#include <coreplugin/core_global.h>

#include <QtGui/QIcon>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT IOptionsPage : public QObject
{
    Q_OBJECT
public:
    IOptionsPage(QObject *parent = 0) : QObject(parent) {}
    virtual ~IOptionsPage() {}

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual QString category() const = 0;
    virtual QString displayCategory() const = 0;
    virtual QIcon categoryIcon() const = 0;
    virtual bool matches(const QString & /* searchKeyWord*/) const { return false; }

    virtual QWidget *createPage(QWidget *parent) = 0;
    virtual void apply() = 0;
    virtual void finish() = 0;
};

/*
    Alternative way for providing option pages instead of adding IOptionsPage
    objects into the plugin manager pool. Should only be used if creation of the
    actual option pages is not possible or too expensive at Qt Creator startup.
    (Like the designer integration, which needs to initialize designer plugins
    before the options pages get available.)
*/

class CORE_EXPORT IOptionsPageProvider : public QObject
{
    Q_OBJECT

public:
    IOptionsPageProvider(QObject *parent = 0) : QObject(parent) {}
    virtual ~IOptionsPageProvider() {}

    virtual QString category() const = 0;
    virtual QString displayCategory() const = 0;
    virtual QIcon categoryIcon() const = 0;

    virtual QList<IOptionsPage *> pages() const = 0;
};

} // namespace Core

#endif // IOPTIONSPAGE_H
