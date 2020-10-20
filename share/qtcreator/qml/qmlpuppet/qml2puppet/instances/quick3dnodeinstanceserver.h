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

#include "changeselectioncommand.h"
#include "qt5nodeinstanceserver.h"
#include "requestmodelnodepreviewimagecommand.h"
#include "tokencommand.h"
#include "valueschangedcommand.h"

#include <QImage>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QDragMoveEvent;
QT_END_NAMESPACE

namespace QmlDesigner {

class Quick3dNodeInstanceServer : public Qt5NodeInstanceServer
{
    Q_OBJECT

public:
    explicit Quick3dNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);
    ~Quick3dNodeInstanceServer() override;

    void reparentInstances(const ReparentInstancesCommand &command) override;
    void update3DViewState(const Update3dViewStateCommand &command) override;
    void createScene(const CreateSceneCommand &command) override;
    void changeSelection(const ChangeSelectionCommand &command) override;
    void changePropertyValues(const ChangeValuesCommand &command) override;
    void removeInstances(const RemoveInstancesCommand &command) override;
    void inputEvent(const InputEventCommand &command) override;
    void view3DAction(const View3DActionCommand &command) override;
    void requestModelNodePreviewImage(const RequestModelNodePreviewImageCommand &command) override;
    void changeAuxiliaryValues(const ChangeAuxiliaryCommand &command) override;
    void changePropertyBindings(const ChangeBindingsCommand &command) override;
    void changeIds(const ChangeIdsCommand &command) override;
    void changeState(const ChangeStateCommand &command) override;
    void removeProperties(const RemovePropertiesCommand &command) override;

private slots:
    void handleSelectionChanged(const QVariant &objs);
    void handleObjectPropertyCommit(const QVariant &object, const QVariant &propName);
    void handleObjectPropertyChange(const QVariant &object, const QVariant &propName);
    void handleActiveSceneChange();
    void handleToolStateChanged(const QString &sceneId, const QString &tool, const QVariant &toolState);
    void handleView3DSizeChange();
    void handleView3DDestroyed(QObject *obj);
    void handleNode3DDestroyed(QObject *obj);

protected:
    void collectItemChangesAndSendChangeCommands() override;
    void selectInstances(const QList<ServerNodeInstance> &instanceList);
    void modifyProperties(const QVector<InstancePropertyValueTriple> &properties);
    QList<ServerNodeInstance> createInstances(const QVector<InstanceContainer> &container) override;
    void initializeAuxiliaryViews() override;

private:
    void handleObjectPropertyChangeTimeout();
    void handleSelectionChangeTimeout();
    void createEditView3D();
    void create3DPreviewView();
    void setup3DEditView(const QList<ServerNodeInstance> &instanceList,
                         const QHash<QString, QVariantMap> &toolStates);
    void createCameraAndLightGizmos(const QList<ServerNodeInstance> &instanceList) const;
    void add3DViewPorts(const QList<ServerNodeInstance> &instanceList);
    void add3DScenes(const QList<ServerNodeInstance> &instanceList);
    QObject *findView3DForInstance(const ServerNodeInstance &instance) const;
    QObject *findView3DForSceneRoot(QObject *sceneRoot) const;
    QObject *find3DSceneRoot(const ServerNodeInstance &instance) const;
    QObject *find3DSceneRoot(QObject *obj) const;
    QVector<InstancePropertyValueTriple> propertyToPropertyValueTriples(
        const ServerNodeInstance &instance, const PropertyName &propertyName, const QVariant &variant);
    void modifyVariantValue(const QVariant &node,
                            const PropertyName &propertyName,
                            TransactionOption option);
    void updateView3DRect(QObject *view3D);
    void updateActiveSceneToEditView3D();
    void removeNode3D(QObject *node);
    void resolveSceneRoots();
    ServerNodeInstance active3DSceneInstance() const;
    void updateNodesRecursive(QQuickItem *item);
    QQuickItem *getContentItemForRendering(QQuickItem *rootItem);
    void render3DEditView(int count = 1);
    void doRender3DEditView();
    void renderModelNodeImageView();
    void doRenderModelNodeImageView();
    void doRenderModelNode3DImageView();
    void doRenderModelNode2DImageView();
    QQuickView *createAuxiliaryQuickView(const QUrl &url, QQuickItem *&rootItem);

    QPointer<QQuickView> m_editView3D;
    QQuickItem *m_editView3DRootItem = nullptr;
    QQuickItem *m_editView3DContentItem = nullptr;
    bool m_editView3DSetupDone = false;
    QPointer<QQuickView> m_ModelNode3DImageView;
    QQuickItem *m_ModelNode3DImageViewRootItem = nullptr;
    QQuickItem *m_ModelNode3DImageViewContentItem = nullptr;
    QPointer<QQuickView> m_ModelNode2DImageView;
    QQuickItem *m_ModelNode2DImageViewRootItem = nullptr;
    QQuickItem *m_ModelNode2DImageViewContentItem = nullptr;
    RequestModelNodePreviewImageCommand m_modelNodePreviewImageCommand;
    QHash<QString, QImage> m_modelNodePreviewImageCache;
    QSet<QObject *> m_view3Ds;
    QMultiHash<QObject *, QObject *> m_3DSceneMap; // key: scene root, value: node
    QObject *m_active3DView = nullptr;
    QObject *m_active3DScene = nullptr;
    bool m_active3DSceneUpdatePending = false;
    QTimer m_propertyChangeTimer;
    QTimer m_selectionChangeTimer;
    QTimer m_render3DEditViewTimer;
    QTimer m_renderModelNodeImageViewTimer;
    QVariant m_changedNode;
    PropertyName m_changedProperty;
    ChangeSelectionCommand m_lastSelectionChangeCommand;
    QObject *m_3dHelper = nullptr;
    int m_need3DEditViewRender = 0;
};

} // namespace QmlDesigner
