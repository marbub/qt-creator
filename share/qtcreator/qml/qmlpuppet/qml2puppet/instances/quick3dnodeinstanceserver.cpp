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

#include "quick3dnodeinstanceserver.h"

#include <QDropEvent>
#include <QMimeData>
#include <QQuickItem>
#include <QQuickView>

#include "changebindingscommand.h"
#include "changefileurlcommand.h"
#include "changeidscommand.h"
#include "changestatecommand.h"
#include "changevaluescommand.h"
#include "childrenchangedcommand.h"
#include "childrenchangeeventfilter.h"
#include "clearscenecommand.h"
#include "commondefines.h"
#include "completecomponentcommand.h"
#include "componentcompletedcommand.h"
#include "createinstancescommand.h"
#include "createscenecommand.h"
#include "informationchangedcommand.h"
#include "inputeventcommand.h"
#include "instancecontainer.h"
#include "nodeinstanceclientinterface.h"
#include "objectnodeinstance.h"
#include "pixmapchangedcommand.h"
#include "propertyabstractcontainer.h"
#include "propertybindingcontainer.h"
#include "propertyvaluecontainer.h"
#include "puppettocreatorcommand.h"
#include "removeinstancescommand.h"
#include "removepropertiescommand.h"
#include "removesharedmemorycommand.h"
#include "reparentinstancescommand.h"
#include "requestmodelnodepreviewimagecommand.h"
#include "servernodeinstance.h"
#include "tokencommand.h"
#include "update3dviewstatecommand.h"
#include "valueschangedcommand.h"
#include "view3dactioncommand.h"

#include "../editor3d/camerageometry.h"
#include "../editor3d/generalhelper.h"
#include "../editor3d/gridgeometry.h"
#include "../editor3d/icongizmoimageprovider.h"
#include "../editor3d/lightgeometry.h"
#include "../editor3d/linegeometry.h"
#include "../editor3d/mousearea3d.h"
#include "../editor3d/selectionboxgeometry.h"
#include "dummycontextobject.h"

#include <designersupportdelegate.h>
#include <qmlprivategate.h>
#include <quickitemnodeinstance.h>

#include <QOpenGLContext>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QQuickView>
#include <QVector3D>
#include <QtGui/qevent.h>
#include <QtGui/qguiapplication.h>

#ifdef QUICK3D_MODULE
#include <QtQuick3D/private/qquick3dabstractlight_p.h>
#include <QtQuick3D/private/qquick3dcamera_p.h>
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dscenerootnode_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "../editor3d/qt5compat/qquick3darealight_p.h"
#endif
#endif

namespace QmlDesigner {

static QVariant objectToVariant(QObject *object)
{
    return QVariant::fromValue(object);
}

static QImage nonVisualComponentPreviewImage()
{
    static double ratio = qgetenv("FORMEDITOR_DEVICE_PIXEL_RATIO").toDouble();
    if (ratio == 1.) {
        static const QImage image(":/qtquickplugin/images/non-visual-component.png");
        return image;
    } else {
        static const QImage image(":/qtquickplugin/images/non-visual-component@2x.png");
        return image;
    }
}

static bool imageHasContent(const QImage &image)
{
    // Check if any image pixel contains non-zero data
    const uchar *pData = image.constBits();
    const qsizetype size = image.sizeInBytes();
    for (qsizetype i = 0; i < size; ++i) {
        if (*(pData++) != 0)
            return true;
    }
    return false;
}

QQuickView *Quick3dNodeInstanceServer::createAuxiliaryQuickView(const QUrl &url, QQuickItem *&rootItem)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto view = new QQuickView(quickView()->engine(), quickView());
    view->setFormat(quickView()->format());
    DesignerSupport::createOpenGLContext(view);
#else
    auto view = new QQuickView(quickView()->engine(), nullptr);
    view->setFormat(quickView()->format());
#endif
    QQmlComponent component(engine());
    component.loadUrl(url);
    rootItem = qobject_cast<QQuickItem *>(component.create());

    if (!rootItem) {
        qWarning() << "Could not create view for: " << url.toString() << component.errors();
        return nullptr;
    }

    DesignerSupport::setRootItem(view, rootItem);

    return view;
}

void Quick3dNodeInstanceServer::createEditView3D()
{
#ifdef QUICK3D_MODULE
    qmlRegisterRevision<QQuick3DNode, 1>("MouseArea3D", 1, 0);
    qmlRegisterType<QmlDesigner::Internal::MouseArea3D>("MouseArea3D", 1, 0, "MouseArea3D");
    qmlRegisterType<QmlDesigner::Internal::CameraGeometry>("CameraGeometry", 1, 0, "CameraGeometry");
    qmlRegisterType<QmlDesigner::Internal::LightGeometry>("LightUtils", 1, 0, "LightGeometry");
    qmlRegisterType<QmlDesigner::Internal::GridGeometry>("GridGeometry", 1, 0, "GridGeometry");
    qmlRegisterType<QmlDesigner::Internal::SelectionBoxGeometry>("SelectionBoxGeometry",
                                                                 1,
                                                                 0,
                                                                 "SelectionBoxGeometry");
    qmlRegisterType<QmlDesigner::Internal::LineGeometry>("LineGeometry", 1, 0, "LineGeometry");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    qmlRegisterType<QmlDesigner::Internal::QQuick3DAreaLight>("LightUtils", 1, 0, "AreaLight");
#endif

    auto helper = new QmlDesigner::Internal::GeneralHelper();
    QObject::connect(helper,
                     &QmlDesigner::Internal::GeneralHelper::toolStateChanged,
                     this,
                     &Quick3dNodeInstanceServer::handleToolStateChanged);
    engine()->rootContext()->setContextProperty("_generalHelper", helper);
    engine()->addImageProvider(QLatin1String("IconGizmoImageProvider"),
                               new QmlDesigner::Internal::IconGizmoImageProvider);
    m_3dHelper = helper;

    m_editView3D = createAuxiliaryQuickView(QUrl("qrc:/qtquickplugin/mockfiles/EditView3D.qml"),
                                            m_editView3DRootItem);

    if (m_editView3DRootItem)
        helper->setParent(m_editView3DRootItem);
#endif
}

// The selection has changed in the edit view 3D. Empty list indicates selection is cleared.
void Quick3dNodeInstanceServer::handleSelectionChanged(const QVariant &objs)
{
    ServerNodeInstances instanceList;
    const QVariantList varObjs = objs.value<QVariantList>();
    for (const auto &object : varObjs) {
        auto obj = object.value<QObject *>();
        if (obj) {
            ServerNodeInstance instance = instanceForObject(obj);
            instanceList << instance;
        }
    }
    selectInstances(instanceList);
    // Hold selection changes reflected back from designer for a bit
    m_selectionChangeTimer.start(500);
}

QVector<Quick3dNodeInstanceServer::InstancePropertyValueTriple>
Quick3dNodeInstanceServer::propertyToPropertyValueTriples(const ServerNodeInstance &instance,
                                                          const PropertyName &propertyName,
                                                          const QVariant &variant)
{
    QVector<InstancePropertyValueTriple> result;
    InstancePropertyValueTriple propTriple;

    if (variant.type() == QVariant::Vector3D) {
        auto vector3d = variant.value<QVector3D>();

        if (vector3d.isNull())
            return result;

        const PropertyName dot = propertyName.isEmpty() ? "" : ".";
        propTriple.instance = instance;
        propTriple.propertyName = propertyName + dot + PropertyName("x");
        propTriple.propertyValue = vector3d.x();
        result.append(propTriple);
        propTriple.propertyName = propertyName + dot + PropertyName("y");
        propTriple.propertyValue = vector3d.y();
        result.append(propTriple);
        propTriple.propertyName = propertyName + dot + PropertyName("z");
        propTriple.propertyValue = vector3d.z();
        result.append(propTriple);
    } else {
        propTriple.instance = instance;
        propTriple.propertyName = propertyName;
        propTriple.propertyValue = variant;
        result.append(propTriple);
    }

    return result;
}

void Quick3dNodeInstanceServer::modifyVariantValue(const QVariant &node,
                                                   const PropertyName &propertyName,
                                                   TransactionOption option)
{
    PropertyName targetPropertyName;

    // Position is a special case, because the position can be 'position.x 'or simply 'x'.
    // We prefer 'x'.
    if (propertyName != "position")
        targetPropertyName = propertyName;

    auto *obj = node.value<QObject *>();

    if (obj) {
        ServerNodeInstance instance = instanceForObject(obj);

        if (option == TransactionOption::Start)
            instance.setModifiedFlag(true);
        else if (option == TransactionOption::End)
            instance.setModifiedFlag(false);

        // We do have to split position into position.x, position.y, position.z
        ValuesModifiedCommand command = createValuesModifiedCommand(
            propertyToPropertyValueTriples(instance, targetPropertyName, obj->property(propertyName)));

        command.transactionOption = option;

        nodeInstanceClient()->valuesModified(command);
    }
}

void Quick3dNodeInstanceServer::handleObjectPropertyCommit(const QVariant &object,
                                                           const QVariant &propName)
{
    modifyVariantValue(object, propName.toByteArray(), TransactionOption::End);
    m_changedNode = {};
    m_changedProperty = {};
    m_propertyChangeTimer.stop();
}

void Quick3dNodeInstanceServer::handleObjectPropertyChange(const QVariant &object,
                                                           const QVariant &propName)
{
    PropertyName propertyName(propName.toByteArray());
    if (m_changedProperty != propertyName || m_changedNode != object) {
        if (!m_changedNode.isNull())
            handleObjectPropertyCommit(m_changedNode, m_changedProperty);
        modifyVariantValue(object, propertyName, TransactionOption::Start);
    } else if (!m_propertyChangeTimer.isActive()) {
        m_propertyChangeTimer.start();
    }
    m_changedNode = object;
    m_changedProperty = propertyName;
}

void Quick3dNodeInstanceServer::handleActiveSceneChange()
{
#ifdef QUICK3D_MODULE
    ServerNodeInstance sceneInstance = active3DSceneInstance();
    const QString sceneId = sceneInstance.id();

    QVariantMap toolStates;
    auto helper = qobject_cast<QmlDesigner::Internal::GeneralHelper *>(m_3dHelper);
    if (helper)
        toolStates = helper->getToolStates(sceneId);
    toolStates.insert("sceneInstanceId", QVariant::fromValue(sceneInstance.instanceId()));

    nodeInstanceClient()->handlePuppetToCreatorCommand(
        {PuppetToCreatorCommand::ActiveSceneChanged, toolStates});
    m_selectionChangeTimer.start(0);
#endif
}

void Quick3dNodeInstanceServer::handleToolStateChanged(const QString &sceneId,
                                                       const QString &tool,
                                                       const QVariant &toolState)
{
    QVariantList data;
    data << sceneId;
    data << tool;
    data << toolState;
    nodeInstanceClient()->handlePuppetToCreatorCommand(
        {PuppetToCreatorCommand::Edit3DToolState, QVariant::fromValue(data)});
}

void Quick3dNodeInstanceServer::handleView3DSizeChange()
{
    QObject *view3D = sender();
    if (view3D == m_active3DView)
        updateView3DRect(view3D);
}

void Quick3dNodeInstanceServer::handleView3DDestroyed(QObject *obj)
{
#ifdef QUICK3D_MODULE
    auto view = qobject_cast<QQuick3DViewport *>(obj);
    m_view3Ds.remove(obj);
    removeNode3D(view->scene());
    if (view && view == m_active3DView)
        m_active3DView = nullptr;
#else
    Q_UNUSED(obj)
#endif
}

void Quick3dNodeInstanceServer::handleNode3DDestroyed(QObject *obj)
{
#ifdef QUICK3D_MODULE
    if (qobject_cast<QQuick3DCamera *>(obj)) {
        QMetaObject::invokeMethod(m_editView3DRootItem,
                                  "releaseCameraGizmo",
                                  Q_ARG(QVariant, objectToVariant(obj)));
    } else if (qobject_cast<QQuick3DAbstractLight *>(obj)) {
        QMetaObject::invokeMethod(m_editView3DRootItem,
                                  "releaseLightGizmo",
                                  Q_ARG(QVariant, objectToVariant(obj)));
    }
    removeNode3D(obj);
#else
    Q_UNUSED(obj)
#endif
}

void Quick3dNodeInstanceServer::updateView3DRect(QObject *view3D)
{
    QRectF viewPortrect(0., 0., 1000., 1000.);
    if (view3D) {
        viewPortrect = QRectF(0.,
                              0.,
                              view3D->property("width").toDouble(),
                              view3D->property("height").toDouble());
    }
    QQmlProperty viewPortProperty(m_editView3DRootItem, "viewPortRect", context());
    viewPortProperty.write(viewPortrect);
}

void Quick3dNodeInstanceServer::updateActiveSceneToEditView3D()
{
#ifdef QUICK3D_MODULE
    if (!m_editView3DSetupDone)
        return;

    // Active scene change handling on qml side is async, so a deleted importScene would crash
    // editView when it updates next. Disable/enable edit view update synchronously to avoid this.
    QVariant activeSceneVar = objectToVariant(m_active3DScene);
    QMetaObject::invokeMethod(m_editView3DRootItem,
                              "enableEditViewUpdate",
                              Q_ARG(QVariant, activeSceneVar));

    ServerNodeInstance sceneInstance = active3DSceneInstance();
    const QString sceneId = sceneInstance.id();

    // QML item id is updated with separate call, so delay this update until we have it
    if (m_active3DScene && sceneId.isEmpty()) {
        m_active3DSceneUpdatePending = true;
        return;
    } else {
        m_active3DSceneUpdatePending = false;
    }

    QMetaObject::invokeMethod(m_editView3DRootItem,
                              "setActiveScene",
                              Qt::QueuedConnection,
                              Q_ARG(QVariant, activeSceneVar),
                              Q_ARG(QVariant, QVariant::fromValue(sceneId)));

    updateView3DRect(m_active3DView);

    auto helper = qobject_cast<QmlDesigner::Internal::GeneralHelper *>(m_3dHelper);
    if (helper)
        helper->storeToolState(helper->globalStateId(), helper->lastSceneIdKey(), QVariant(sceneId), 0);
#endif
}

void Quick3dNodeInstanceServer::removeNode3D(QObject *node)
{
    m_3DSceneMap.remove(node);
    const auto oldMap = m_3DSceneMap;
    auto it = oldMap.constBegin();
    while (it != oldMap.constEnd()) {
        if (it.value() == node) {
            m_3DSceneMap.remove(it.key(), node);
            break;
        }
        ++it;
    }
    if (node == m_active3DScene) {
        m_active3DScene = nullptr;
        m_active3DView = nullptr;
        updateActiveSceneToEditView3D();
    }
}

void Quick3dNodeInstanceServer::resolveSceneRoots()
{
#ifdef QUICK3D_MODULE
    if (!m_editView3DSetupDone)
        return;

    const auto oldMap = m_3DSceneMap;
    m_3DSceneMap.clear();
    auto it = oldMap.begin();
    bool updateActiveScene = !m_active3DScene;
    while (it != oldMap.end()) {
        QObject *node = it.value();
        QObject *newRoot = find3DSceneRoot(node);
        QObject *oldRoot = it.key();
        if (!m_active3DScene || (newRoot != oldRoot && m_active3DScene == oldRoot)) {
            m_active3DScene = newRoot;
            updateActiveScene = true;
        }
        m_3DSceneMap.insert(newRoot, node);

        if (newRoot != oldRoot) {
            if (qobject_cast<QQuick3DCamera *>(node)) {
                QMetaObject::invokeMethod(m_editView3DRootItem,
                                          "updateCameraGizmoScene",
                                          Q_ARG(QVariant, objectToVariant(newRoot)),
                                          Q_ARG(QVariant, objectToVariant(node)));
            } else if (qobject_cast<QQuick3DAbstractLight *>(node)) {
                QMetaObject::invokeMethod(m_editView3DRootItem,
                                          "updateLightGizmoScene",
                                          Q_ARG(QVariant, objectToVariant(newRoot)),
                                          Q_ARG(QVariant, objectToVariant(node)));
            }
        }
        ++it;
    }
    if (updateActiveScene) {
        m_active3DView = findView3DForSceneRoot(m_active3DScene);
        updateActiveSceneToEditView3D();
    }
#endif
}

ServerNodeInstance Quick3dNodeInstanceServer::active3DSceneInstance() const
{
    ServerNodeInstance sceneInstance;
    if (hasInstanceForObject(m_active3DScene))
        sceneInstance = instanceForObject(m_active3DScene);
    else if (hasInstanceForObject(m_active3DView))
        sceneInstance = instanceForObject(m_active3DView);
    return sceneInstance;
}

void Quick3dNodeInstanceServer::updateNodesRecursive(QQuickItem *item)
{
    const auto childItems = item->childItems();
    for (QQuickItem *childItem : childItems)
        updateNodesRecursive(childItem);
    if (Internal::QuickItemNodeInstance::unifiedRenderPath()) {
        if (item->flags() & QQuickItem::ItemHasContents)
            item->update();
    } else {
        DesignerSupport::updateDirtyNode(item);
    }
}

QQuickItem *Quick3dNodeInstanceServer::getContentItemForRendering(QQuickItem *rootItem)
{
    QQuickItem *contentItem = QQmlProperty::read(rootItem, "contentItem").value<QQuickItem *>();
    if (contentItem) {
        if (!Internal::QuickItemNodeInstance::unifiedRenderPath())
            designerSupport()->refFromEffectItem(contentItem, false);
        QmlDesigner::Internal::QmlPrivateGate::disableNativeTextRendering(contentItem);
    }
    return contentItem;
}

void Quick3dNodeInstanceServer::render3DEditView(int count)
{
    m_need3DEditViewRender = qMax(count, m_need3DEditViewRender);
    if (!m_render3DEditViewTimer.isActive())
        m_render3DEditViewTimer.start(0);
}

// render the 3D edit view and send the result to creator process
void Quick3dNodeInstanceServer::doRender3DEditView()
{
    if (m_editView3DSetupDone) {
        if (!m_editView3DContentItem)
            m_editView3DContentItem = getContentItemForRendering(m_editView3DRootItem);

        QImage renderImage;

        updateNodesRecursive(m_editView3DContentItem);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (Internal::QuickItemNodeInstance::unifiedRenderPath()) {
            renderImage = m_editView3D->grabWindow();
        } else {
            // Fake render loop signaling to update things like QML items as 3D textures
            m_editView3D->beforeSynchronizing();
            m_editView3D->beforeRendering();

            QSizeF size = qobject_cast<QQuickItem *>(m_editView3DContentItem)->size();
            QRectF renderRect(QPointF(0., 0.), size);
            renderImage = designerSupport()->renderImageForItem(m_editView3DContentItem,
                                                                renderRect,
                                                                size.toSize());

            m_editView3D->afterRendering();
        }
#else
        renderImage = m_editView3D->grabWindow();
#endif

        // There's no instance related to image, so instance id is -1.
        // Key number is selected so that it is unlikely to conflict other ImageContainer use.
        auto imgContainer = ImageContainer(-1, renderImage, 2100000000);

        // send the rendered image to creator process
        nodeInstanceClient()->handlePuppetToCreatorCommand(
            {PuppetToCreatorCommand::Render3DView, QVariant::fromValue(imgContainer)});
        if (m_need3DEditViewRender > 0) {
            m_render3DEditViewTimer.start(0);
            --m_need3DEditViewRender;
        }
    }
}

void Quick3dNodeInstanceServer::renderModelNodeImageView()
{
    if (!m_renderModelNodeImageViewTimer.isActive())
        m_renderModelNodeImageViewTimer.start(0);
}

void Quick3dNodeInstanceServer::doRenderModelNodeImageView()
{
    ServerNodeInstance instance;
    if (m_modelNodePreviewImageCommand.renderItemId() >= 0)
        instance = instanceForId(m_modelNodePreviewImageCommand.renderItemId());
    else
        instance = instanceForId(m_modelNodePreviewImageCommand.instanceId());

    if (instance.isSubclassOf("QQuick3DObject"))
        doRenderModelNode3DImageView();
    else if (instance.isSubclassOf("QQuickItem"))
        doRenderModelNode2DImageView();
}

void Quick3dNodeInstanceServer::doRenderModelNode3DImageView()
{
#ifdef QUICK3D_MODULE
    if (m_ModelNode3DImageViewRootItem) {
        if (!m_ModelNode3DImageViewContentItem)
            m_ModelNode3DImageViewContentItem = getContentItemForRendering(
                m_ModelNode3DImageViewRootItem);

        // Key number is selected so that it is unlikely to conflict other ImageContainer use.
        auto imgContainer = ImageContainer(m_modelNodePreviewImageCommand.instanceId(), {}, 2100000001);
        QImage renderImage;
        if (m_modelNodePreviewImageCache.contains(m_modelNodePreviewImageCommand.componentPath())) {
            renderImage = m_modelNodePreviewImageCache[m_modelNodePreviewImageCommand.componentPath()];
        } else {
            QObject *instanceObj = nullptr;
            if (!m_modelNodePreviewImageCommand.componentPath().isEmpty()) {
                QQmlComponent component(engine());
                component.loadUrl(QUrl::fromLocalFile(m_modelNodePreviewImageCommand.componentPath()));
                instanceObj = qobject_cast<QQuick3DObject *>(component.create());
                if (!instanceObj) {
                    qWarning() << "Could not create preview component: " << component.errors();
                    return;
                }
            } else {
                ServerNodeInstance instance = instanceForId(
                    m_modelNodePreviewImageCommand.instanceId());
                instanceObj = instance.internalObject();
            }
            QSize renderSize = m_modelNodePreviewImageCommand.size();
            if (Internal::QuickItemNodeInstance::unifiedRenderPath()) {
                // Requested size is already adjusted for target pixel ratio, so we have to adjust
                // back if ratio is not default for our window.
                double ratio = m_ModelNode3DImageView->devicePixelRatio();
                renderSize.setWidth(qRound(qreal(renderSize.width()) / ratio));
                renderSize.setHeight(qRound(qreal(renderSize.height()) / ratio));
            }

            QMetaObject::invokeMethod(m_ModelNode3DImageViewRootItem,
                                      "createViewForObject",
                                      Q_ARG(QVariant, objectToVariant(instanceObj)),
                                      Q_ARG(QVariant, QVariant::fromValue(renderSize.width())),
                                      Q_ARG(QVariant, QVariant::fromValue(renderSize.height())));

            bool ready = false;
            int count = 0; // Ensure we don't ever get stuck in an infinite loop
            while (!ready && ++count < 10) {
                updateNodesRecursive(m_ModelNode3DImageViewContentItem);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                if (Internal::QuickItemNodeInstance::unifiedRenderPath()) {
                    renderImage = m_ModelNode3DImageView->grabWindow();
                } else {
                    // Fake render loop signaling to update things like QML items as 3D textures
                    m_ModelNode3DImageView->beforeSynchronizing();
                    m_ModelNode3DImageView->beforeRendering();

                    QSizeF size = qobject_cast<QQuickItem *>(m_ModelNode3DImageViewContentItem)->size();
                    QRectF renderRect(QPointF(0., 0.), size);
                    renderImage = designerSupport()->renderImageForItem(m_ModelNode3DImageViewContentItem,
                                                                        renderRect,
                                                                        size.toSize());

                    m_ModelNode3DImageView->afterRendering();
                }
#else
                renderImage = m_ModelNode3DImageView->grabWindow();
#endif
                QMetaObject::invokeMethod(m_ModelNode3DImageViewRootItem, "afterRender");
                ready = QQmlProperty::read(m_ModelNode3DImageViewRootItem, "ready").value<bool>();
            }
            QMetaObject::invokeMethod(m_ModelNode3DImageViewRootItem, "destroyView");
            if (!m_modelNodePreviewImageCommand.componentPath().isEmpty()) {
                // If component changes, puppet will need a reset anyway, so we can cache the image
                m_modelNodePreviewImageCache.insert(m_modelNodePreviewImageCommand.componentPath(),
                                                    renderImage);
                delete instanceObj;
            }
        }

        if (!renderImage.isNull()) {
            imgContainer.setImage(renderImage);

            // send the rendered image to creator process
            nodeInstanceClient()->handlePuppetToCreatorCommand(
                {PuppetToCreatorCommand::RenderModelNodePreviewImage,
                 QVariant::fromValue(imgContainer)});
        }
    }
#endif
}

static QRectF itemBoundingRect(QQuickItem *item)
{
    QRectF itemRect;
    if (item) {
        itemRect = item->boundingRect();
        if (item->clip()) {
            return itemRect;
        } else {
            const auto childItems = item->childItems();
            for (const auto &childItem : childItems) {
                QRectF mappedRect = childItem->mapRectToItem(item, itemBoundingRect(childItem));
                // Sanity check for size
                if (mappedRect.isValid() && (mappedRect.width() < 10000)
                    && (mappedRect.height() < 10000))
                    itemRect = itemRect.united(mappedRect);
            }
        }
    }
    return itemRect;
}

void Quick3dNodeInstanceServer::doRenderModelNode2DImageView()
{
    if (m_ModelNode2DImageViewRootItem) {
        if (!m_ModelNode2DImageViewContentItem)
            m_ModelNode2DImageViewContentItem = getContentItemForRendering(
                m_ModelNode2DImageViewRootItem);

        // Key number is the same as in 3D case as they produce image for same purpose
        auto imgContainer = ImageContainer(m_modelNodePreviewImageCommand.instanceId(), {}, 2100000001);
        QImage renderImage;
        if (m_modelNodePreviewImageCache.contains(m_modelNodePreviewImageCommand.componentPath())) {
            renderImage = m_modelNodePreviewImageCache[m_modelNodePreviewImageCommand.componentPath()];
        } else {
            QQuickItem *instanceItem = nullptr;

            if (!m_modelNodePreviewImageCommand.componentPath().isEmpty()) {
                QQmlComponent component(engine());
                component.loadUrl(QUrl::fromLocalFile(m_modelNodePreviewImageCommand.componentPath()));
                instanceItem = qobject_cast<QQuickItem *>(component.create());
                if (!instanceItem) {
                    qWarning() << "Could not create preview component: " << component.errors();
                    return;
                }
            } else {
                qWarning() << "2D image preview is not supported for non-components.";
                return;
            }

            instanceItem->setParentItem(m_ModelNode2DImageViewContentItem);

            // Some component may expect to always be shown at certain size, so their layouts may
            // not support scaling, so let's always render at the default size if item has one and
            // scale the resulting image instead.
            QSize finalSize = m_modelNodePreviewImageCommand.size();
            QRectF renderRect = itemBoundingRect(instanceItem);
            QSize renderSize = renderRect.size().toSize();
            if (renderSize.isEmpty()) {
                renderSize = finalSize;
                renderRect = QRectF(QPointF(0., 0.), QSizeF(renderSize));
            }
            m_ModelNode2DImageView->resize(renderSize);
            m_ModelNode2DImageViewRootItem->setSize(renderSize);
            m_ModelNode2DImageViewContentItem->setPosition(QPointF(-renderRect.x(), -renderRect.y()));

            updateNodesRecursive(m_ModelNode2DImageViewContentItem);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            if (Internal::QuickItemNodeInstance::unifiedRenderPath()) {
                renderImage = m_ModelNode2DImageView->grabWindow();
            } else {
                renderImage = designerSupport()->renderImageForItem(m_ModelNode2DImageViewContentItem,
                                                                    renderRect,
                                                                    renderSize);
            }
#else
            renderImage = m_ModelNode2DImageView->grabWindow();
#endif

            if (!imageHasContent(renderImage))
                renderImage = nonVisualComponentPreviewImage();

            if (renderSize != finalSize)
                renderImage = renderImage.scaled(finalSize, Qt::KeepAspectRatio);

            delete instanceItem;

            // If component changes, puppet will need a reset anyway, so we can cache the image
            m_modelNodePreviewImageCache.insert(m_modelNodePreviewImageCommand.componentPath(),
                                                renderImage);
        }

        if (!renderImage.isNull()) {
            imgContainer.setImage(renderImage);

            // send the rendered image to creator process
            nodeInstanceClient()->handlePuppetToCreatorCommand(
                {PuppetToCreatorCommand::RenderModelNodePreviewImage,
                 QVariant::fromValue(imgContainer)});
        }
    }
}

Quick3dNodeInstanceServer::Quick3dNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient)
    : Qt5NodeInstanceServer(nodeInstanceClient)
{
    m_propertyChangeTimer.setInterval(100);
    m_propertyChangeTimer.setSingleShot(true);
    m_selectionChangeTimer.setSingleShot(true);
    m_render3DEditViewTimer.setSingleShot(true);
    m_renderModelNodeImageViewTimer.setSingleShot(true);
}

Quick3dNodeInstanceServer::~Quick3dNodeInstanceServer()
{
    for (auto view : qAsConst(m_view3Ds))
        QObject::disconnect(view, nullptr, this, nullptr);
}

/* This method allows changing the selection from the puppet */
void Quick3dNodeInstanceServer::selectInstances(const ServerNodeInstances &instanceList)
{
    nodeInstanceClient()->selectionChanged(createChangeSelectionCommand(instanceList));
}

/* This method allows changing property values from the puppet
 * For performance reasons (and the undo stack) properties should always be modifed in 'bulks'.
 */
void Quick3dNodeInstanceServer::modifyProperties(
    const QVector<NodeInstanceServer::InstancePropertyValueTriple> &properties)
{
    nodeInstanceClient()->valuesModified(createValuesModifiedCommand(properties));
}

ServerNodeInstances Quick3dNodeInstanceServer::createInstances(const QVector<InstanceContainer> &container)
{
    const auto createdInstances = NodeInstanceServer::createInstances(container);

    if (m_editView3DSetupDone) {
        add3DViewPorts(createdInstances);
        add3DScenes(createdInstances);
        createCameraAndLightGizmos(createdInstances);
    }

    render3DEditView();

    return createdInstances;
}

void Quick3dNodeInstanceServer::initializeAuxiliaryViews()
{
#ifdef QUICK3D_MODULE
    if (qEnvironmentVariableIsSet("QMLDESIGNER_QUICK3D_MODE")) {
        createEditView3D();
        m_ModelNode3DImageView = createAuxiliaryQuickView(
            QUrl("qrc:/qtquickplugin/mockfiles/ModelNode3DImageView.qml"),
            m_ModelNode3DImageViewRootItem);
    }
#endif

    m_ModelNode2DImageView = createAuxiliaryQuickView(
        QUrl("qrc:/qtquickplugin/mockfiles/ModelNode2DImageView.qml"), m_ModelNode2DImageViewRootItem);
    m_ModelNode2DImageView->setDefaultAlphaBuffer(true);
    m_ModelNode2DImageView->setColor(Qt::transparent);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!m_editView3D.isNull()) {
        m_editView3D->show();
        m_editView3D->lower();
    }
    if (!m_ModelNode3DImageView.isNull()) {
        m_ModelNode3DImageView->show();
        m_ModelNode3DImageView->lower();
    }
    if (!m_ModelNode2DImageView.isNull()) {
        m_ModelNode2DImageView->show();
        m_ModelNode2DImageView->lower();
    }
#endif
}

void Quick3dNodeInstanceServer::handleObjectPropertyChangeTimeout()
{
    modifyVariantValue(m_changedNode, m_changedProperty, TransactionOption::None);
}

void Quick3dNodeInstanceServer::handleSelectionChangeTimeout()
{
    changeSelection(m_lastSelectionChangeCommand);
}

void Quick3dNodeInstanceServer::createCameraAndLightGizmos(const ServerNodeInstances &instanceList) const
{
    QHash<QObject *, QObjectList> cameras;
    QHash<QObject *, QObjectList> lights;

    for (const ServerNodeInstance &instance : instanceList) {
        if (instance.isSubclassOf("QQuick3DCamera"))
            cameras[find3DSceneRoot(instance)] << instance.internalObject();
        else if (instance.isSubclassOf("QQuick3DAbstractLight"))
            lights[find3DSceneRoot(instance)] << instance.internalObject();
    }

    auto cameraIt = cameras.constBegin();
    while (cameraIt != cameras.constEnd()) {
        const auto cameraObjs = cameraIt.value();
        for (auto &obj : cameraObjs) {
            QMetaObject::invokeMethod(m_editView3DRootItem,
                                      "addCameraGizmo",
                                      Q_ARG(QVariant, objectToVariant(cameraIt.key())),
                                      Q_ARG(QVariant, objectToVariant(obj)));
        }
        ++cameraIt;
    }
    auto lightIt = lights.constBegin();
    while (lightIt != lights.constEnd()) {
        const auto lightObjs = lightIt.value();
        for (auto &obj : lightObjs) {
            QMetaObject::invokeMethod(m_editView3DRootItem,
                                      "addLightGizmo",
                                      Q_ARG(QVariant, objectToVariant(lightIt.key())),
                                      Q_ARG(QVariant, objectToVariant(obj)));
        }
        ++lightIt;
    }
}

void Quick3dNodeInstanceServer::add3DViewPorts(const ServerNodeInstances &instanceList)
{
    for (const ServerNodeInstance &instance : instanceList) {
        if (instance.isSubclassOf("QQuick3DViewport")) {
            QObject *obj = instance.internalObject();
            if (!m_view3Ds.contains(obj)) {
                m_view3Ds << obj;
                QObject::connect(obj, SIGNAL(widthChanged()), this, SLOT(handleView3DSizeChange()));
                QObject::connect(obj, SIGNAL(heightChanged()), this, SLOT(handleView3DSizeChange()));
                QObject::connect(obj,
                                 &QObject::destroyed,
                                 this,
                                 &Quick3dNodeInstanceServer::handleView3DDestroyed);
            }
        }
    }
}

void Quick3dNodeInstanceServer::add3DScenes(const ServerNodeInstances &instanceList)
{
    for (const ServerNodeInstance &instance : instanceList) {
        if (instance.isSubclassOf("QQuick3DNode")) {
            QObject *sceneRoot = find3DSceneRoot(instance);
            QObject *obj = instance.internalObject();
            if (!m_3DSceneMap.contains(sceneRoot, obj)) {
                m_3DSceneMap.insert(sceneRoot, obj);
                QObject::connect(obj,
                                 &QObject::destroyed,
                                 this,
                                 &Quick3dNodeInstanceServer::handleNode3DDestroyed);
            }
        }
    }
}

QObject *Quick3dNodeInstanceServer::findView3DForInstance(const ServerNodeInstance &instance) const
{
#ifdef QUICK3D_MODULE
    if (!instance.isValid())
        return {};

    // View3D of an instance is one of the following, in order of priority:
    // - Any direct ancestor View3D of the instance
    // - Any View3D that specifies the instance's scene as importScene
    ServerNodeInstance checkInstance = instance;
    while (checkInstance.isValid()) {
        if (checkInstance.isSubclassOf("QQuick3DViewport"))
            return checkInstance.internalObject();
        else
            checkInstance = checkInstance.parent();
    }

    // If no ancestor View3D was found, check if the scene root is specified as importScene in
    // some View3D.
    QObject *sceneRoot = find3DSceneRoot(instance);
    for (const auto &view3D : qAsConst(m_view3Ds)) {
        auto view = qobject_cast<QQuick3DViewport *>(view3D);
        if (view && sceneRoot == view->importScene())
            return view3D;
    }
#else
    Q_UNUSED(instance)
#endif
    return {};
}

QObject *Quick3dNodeInstanceServer::findView3DForSceneRoot(QObject *sceneRoot) const
{
#ifdef QUICK3D_MODULE
    if (!sceneRoot)
        return {};

    if (hasInstanceForObject(sceneRoot)) {
        return findView3DForInstance(instanceForObject(sceneRoot));
    } else {
        // No instance, so the scene root must be scene property of one of the views
        for (const auto &view3D : qAsConst(m_view3Ds)) {
            auto view = qobject_cast<QQuick3DViewport *>(view3D);
            if (view && sceneRoot == view->scene())
                return view3D;
        }
    }
#else
    Q_UNUSED(sceneRoot)
#endif
    return {};
}

QObject *Quick3dNodeInstanceServer::find3DSceneRoot(const ServerNodeInstance &instance) const
{
#ifdef QUICK3D_MODULE
    // The root of a 3D scene is any QQuick3DNode that doesn't have QQuick3DNode as parent.
    // One exception is QQuick3DSceneRootNode that has only a single child QQuick3DNode (not
    // a subclass of one, but exactly QQuick3DNode). In that case we consider the single child node
    // to be the scene root (as QQuick3DSceneRootNode is not visible in the navigator scene graph).

    if (!instance.isValid())
        return nullptr;

    QQuick3DNode *childNode = nullptr;
    auto countChildNodes = [&childNode](QQuick3DViewport *view) -> int {
        QQuick3DNode *sceneNode = view->scene();
        QList<QQuick3DObject *> children = sceneNode->childItems();
        int nodeCount = 0;
        for (const auto &child : children) {
            auto nodeChild = qobject_cast<QQuick3DNode *>(child);
            if (nodeChild) {
                ++nodeCount;
                childNode = nodeChild;
            }
        }
        return nodeCount;
    };

    // In case of View3D is selected, the root scene is whatever is contained in View3D, or
    // importScene, in case there is no content in View3D
    QObject *obj = instance.internalObject();
    auto view = qobject_cast<QQuick3DViewport *>(obj);
    if (view) {
        int nodeCount = countChildNodes(view);
        if (nodeCount == 0)
            return view->importScene();
        else if (nodeCount == 1)
            return childNode;
        else
            return view->scene();
    }

    ServerNodeInstance checkInstance = instance;
    bool foundNode = checkInstance.isSubclassOf("QQuick3DNode");
    while (checkInstance.isValid()) {
        ServerNodeInstance parentInstance = checkInstance.parent();
        if (parentInstance.isSubclassOf("QQuick3DViewport")) {
            view = qobject_cast<QQuick3DViewport *>(parentInstance.internalObject());
            int nodeCount = countChildNodes(view);
            if (nodeCount == 1)
                return childNode;
            else
                return view->scene();
        } else if (parentInstance.isSubclassOf("QQuick3DNode")) {
            foundNode = true;
            checkInstance = parentInstance;
        } else {
            if (!foundNode) {
                // We haven't found any node yet, continue the search
                checkInstance = parentInstance;
            } else {
                return checkInstance.internalObject();
            }
        }
    }
#else
    Q_UNUSED(instance)
#endif
    return nullptr;
}

QObject *Quick3dNodeInstanceServer::find3DSceneRoot(QObject *obj) const
{
#ifdef QUICK3D_MODULE
    if (hasInstanceForObject(obj))
        return find3DSceneRoot(instanceForObject(obj));

    // If there is no instance, obj could be a scene in a View3D
    for (const auto &viewObj : qAsConst(m_view3Ds)) {
        const auto view = qobject_cast<QQuick3DViewport *>(viewObj);
        if (view && view->scene() == obj)
            return obj;
    }
#else
    Q_UNUSED(obj)
#endif
    // Some other non-instance object, assume it's not part of any scene
    return nullptr;
}

void Quick3dNodeInstanceServer::setup3DEditView(const ServerNodeInstances &instanceList,
                                                const QHash<QString, QVariantMap> &toolStates)
{
#ifdef QUICK3D_MODULE
    if (!m_editView3DRootItem)
        return;

    ServerNodeInstance root = rootNodeInstance();

    add3DViewPorts(instanceList);
    add3DScenes(instanceList);

    QObject::connect(m_editView3DRootItem,
                     SIGNAL(selectionChanged(QVariant)),
                     this,
                     SLOT(handleSelectionChanged(QVariant)));
    QObject::connect(m_editView3DRootItem,
                     SIGNAL(commitObjectProperty(QVariant, QVariant)),
                     this,
                     SLOT(handleObjectPropertyCommit(QVariant, QVariant)));
    QObject::connect(m_editView3DRootItem,
                     SIGNAL(changeObjectProperty(QVariant, QVariant)),
                     this,
                     SLOT(handleObjectPropertyChange(QVariant, QVariant)));
    QObject::connect(m_editView3DRootItem,
                     SIGNAL(notifyActiveSceneChange()),
                     this,
                     SLOT(handleActiveSceneChange()));
    QObject::connect(&m_propertyChangeTimer,
                     &QTimer::timeout,
                     this,
                     &Quick3dNodeInstanceServer::handleObjectPropertyChangeTimeout);
    QObject::connect(&m_selectionChangeTimer,
                     &QTimer::timeout,
                     this,
                     &Quick3dNodeInstanceServer::handleSelectionChangeTimeout);
    QObject::connect(&m_render3DEditViewTimer,
                     &QTimer::timeout,
                     this,
                     &Quick3dNodeInstanceServer::doRender3DEditView);

    QString lastSceneId;
    auto helper = qobject_cast<QmlDesigner::Internal::GeneralHelper *>(m_3dHelper);
    if (helper) {
        auto it = toolStates.constBegin();
        while (it != toolStates.constEnd()) {
            helper->initToolStates(it.key(), it.value());
            ++it;
        }
        if (toolStates.contains(helper->globalStateId())) {
            if (toolStates[helper->globalStateId()].contains(helper->rootSizeKey()))
                m_editView3DRootItem->setSize(
                    toolStates[helper->globalStateId()][helper->rootSizeKey()].value<QSize>());
            if (toolStates[helper->globalStateId()].contains(helper->lastSceneIdKey()))
                lastSceneId = toolStates[helper->globalStateId()][helper->lastSceneIdKey()].toString();
        }
    }

    // Find a scene to show
    m_active3DScene = nullptr;
    m_active3DView = nullptr;
    if (!m_3DSceneMap.isEmpty()) {
        // Restore the previous scene if possible
        if (!lastSceneId.isEmpty()) {
            const auto keys = m_3DSceneMap.uniqueKeys();
            for (const auto key : keys) {
                m_active3DScene = key;
                m_active3DView = findView3DForSceneRoot(m_active3DScene);
                ServerNodeInstance sceneInstance = active3DSceneInstance();
                if (lastSceneId == sceneInstance.id())
                    break;
            }
        } else {
            m_active3DScene = m_3DSceneMap.begin().key();
            m_active3DView = findView3DForSceneRoot(m_active3DScene);
        }
    }

    m_editView3DSetupDone = true;

    if (toolStates.contains({})) {
        // Update tool state to an existing no-scene state before updating the active scene to
        // ensure the previous state is inherited properly in all cases.
        QMetaObject::invokeMethod(m_editView3DRootItem,
                                  "updateToolStates",
                                  Qt::QueuedConnection,
                                  Q_ARG(QVariant, toolStates[{}]),
                                  Q_ARG(QVariant, QVariant::fromValue(false)));
    }

    updateActiveSceneToEditView3D();

    createCameraAndLightGizmos(instanceList);

    // Queue two renders to make sure icon gizmos update properly
    render3DEditView(2);
#else
    Q_UNUSED(instanceList)
    Q_UNUSED(toolStates)
#endif
}

void Quick3dNodeInstanceServer::collectItemChangesAndSendChangeCommands() {}

void Quick3dNodeInstanceServer::reparentInstances(const ReparentInstancesCommand &command)
{
    Qt5NodeInstanceServer::reparentInstances(command);

    if (m_editView3DSetupDone)
        resolveSceneRoots();

    // Make sure selection is in sync after all reparentings are done
    m_selectionChangeTimer.start(0);
}

void Quick3dNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    Qt5NodeInstanceServer::createScene(command);

    ServerNodeInstances instanceList;
    for (const InstanceContainer &container : command.instances) {
        if (hasInstanceForId(container.instanceId)) {
            ServerNodeInstance instance = instanceForId(container.instanceId);
            if (instance.isValid())
                instanceList.append(instance);
        }
    }

    if (qEnvironmentVariableIsSet("QMLDESIGNER_QUICK3D_MODE"))
        setup3DEditView(instanceList, command.edit3dToolStates);

    QObject::connect(&m_renderModelNodeImageViewTimer,
                     &QTimer::timeout,
                     this,
                     &Quick3dNodeInstanceServer::doRenderModelNodeImageView);
}

void Quick3dNodeInstanceServer::changeSelection(const ChangeSelectionCommand &command)
{
    if (!m_editView3DSetupDone)
        return;

    m_lastSelectionChangeCommand = command;
    if (m_selectionChangeTimer.isActive()) {
        // If selection was recently changed by puppet, hold updating the selection for a bit to
        // avoid selection flicker, especially in multiselect cases.
        // Add additional time in case more commands are still coming through
        m_selectionChangeTimer.start(500);
        return;
    }

    // Find a scene root of the selection to update active scene shown
    const QVector<qint32> instanceIds = command.instanceIds();
    QVariantList selectedObjs;
    QObject *firstSceneRoot = nullptr;
    ServerNodeInstance firstInstance;
    for (qint32 id : instanceIds) {
        if (hasInstanceForId(id)) {
            ServerNodeInstance instance = instanceForId(id);
            QObject *sceneRoot = find3DSceneRoot(instance);
            if (!firstSceneRoot && sceneRoot) {
                firstSceneRoot = sceneRoot;
                firstInstance = instance;
            }
            QObject *object = nullptr;
            if (firstSceneRoot && sceneRoot == firstSceneRoot && instance.isSubclassOf("QQuick3DNode"))
                object = instance.internalObject();

            auto isSelectableAsRoot = [&]() -> bool {
#ifdef QUICK3D_MODULE
                if (qobject_cast<QQuick3DModel *>(object) || qobject_cast<QQuick3DCamera *>(object)
                    || qobject_cast<QQuick3DAbstractLight *>(object)) {
                    return true;
                }
                // Node is a component if it has node children that have no instances
                auto node = qobject_cast<QQuick3DNode *>(object);
                if (node) {
                    const auto childItems = node->childItems();
                    for (const auto &childItem : childItems) {
                        if (qobject_cast<QQuick3DNode *>(childItem)
                            && !hasInstanceForObject(childItem))
                            return true;
                    }
                }
#endif
                return false;
            };
            if (object && (firstSceneRoot != object || isSelectableAsRoot()))
                selectedObjs << objectToVariant(object);
        }
    }

    if (firstSceneRoot && m_active3DScene != firstSceneRoot) {
        m_active3DScene = firstSceneRoot;
        m_active3DView = findView3DForInstance(firstInstance);
        updateActiveSceneToEditView3D();
    }

    // Ensure the UI has enough selection box items. If it doesn't yet have them, which can be the
    // case when the first selection processed is a multiselection, we wait a bit as
    // using the new boxes immediately leads to visual glitches.
    int boxCount = m_editView3DRootItem->property("selectionBoxes").value<QVariantList>().size();
    if (boxCount < selectedObjs.size()) {
        QMetaObject::invokeMethod(m_editView3DRootItem,
                                  "ensureSelectionBoxes",
                                  Q_ARG(QVariant, QVariant::fromValue(selectedObjs.size())));
        m_selectionChangeTimer.start(0);
    } else {
        QMetaObject::invokeMethod(m_editView3DRootItem,
                                  "selectObjects",
                                  Q_ARG(QVariant, QVariant::fromValue(selectedObjs)));
    }

    render3DEditView(2);
}

void Quick3dNodeInstanceServer::changePropertyValues(const ChangeValuesCommand &command)
{
    bool hasDynamicProperties = false;
    const QVector<PropertyValueContainer> values = command.valueChanges();
    for (const PropertyValueContainer &container : values) {
        if (!container.isReflected()) {
            hasDynamicProperties |= container.isDynamic();
            setInstancePropertyVariant(container);
        }
    }

    if (hasDynamicProperties)
        refreshBindings();

    startRenderTimer();

    render3DEditView();
}

void Quick3dNodeInstanceServer::removeInstances(const RemoveInstancesCommand &command)
{
    int nodeCount = m_3DSceneMap.size();

    Qt5NodeInstanceServer::removeInstances(command);

    if (nodeCount != m_3DSceneMap.size()) {
        // Some nodes were removed, which can cause scene root to change for nodes under View3D
        // objects, so re-resolve scene roots.
        resolveSceneRoots();
    }

    if (m_editView3DSetupDone && (!m_active3DScene || !m_active3DView)) {
        if (!m_active3DScene && !m_3DSceneMap.isEmpty())
            m_active3DScene = m_3DSceneMap.begin().key();
        m_active3DView = findView3DForSceneRoot(m_active3DScene);
        updateActiveSceneToEditView3D();
    }
    render3DEditView();
}

void Quick3dNodeInstanceServer::inputEvent(const InputEventCommand &command)
{
    if (m_editView3D) {
        if (command.type() == QEvent::Wheel) {
            QWheelEvent *we
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
                = new QWheelEvent(command.pos(),
                                  command.pos(),
                                  {0, 0},
                                  {0, command.angleDelta()},
                                  command.buttons(),
                                  command.modifiers(),
                                  Qt::NoScrollPhase,
                                  false);
#else
                = new QWheelEvent(command.pos(),
                                  command.pos(),
                                  {0, 0},
                                  {0, command.angleDelta()},
                                  0,
                                  Qt::Horizontal,
                                  command.buttons(),
                                  command.modifiers(),
                                  Qt::NoScrollPhase,
                                  Qt::MouseEventNotSynthesized);
#endif

            QGuiApplication::postEvent(m_editView3D, we);
        } else {
            auto me = new QMouseEvent(command.type(),
                                      command.pos(),
                                      command.button(),
                                      command.buttons(),
                                      command.modifiers());
            QGuiApplication::postEvent(m_editView3D, me);
        }

        render3DEditView();
    }
}

void Quick3dNodeInstanceServer::view3DAction(const View3DActionCommand &command)
{
    if (!m_editView3DSetupDone)
        return;

    QVariantMap updatedState;
    int renderCount = 1;

    switch (command.type()) {
    case View3DActionCommand::MoveTool:
        updatedState.insert("transformMode", 0);
        break;
    case View3DActionCommand::RotateTool:
        updatedState.insert("transformMode", 1);
        break;
    case View3DActionCommand::ScaleTool:
        updatedState.insert("transformMode", 2);
        break;
    case View3DActionCommand::FitToView:
        QMetaObject::invokeMethod(m_editView3DRootItem, "fitToView");
        break;
    case View3DActionCommand::SelectionModeToggle:
        updatedState.insert("selectionMode", command.isEnabled() ? 1 : 0);
        break;
    case View3DActionCommand::CameraToggle:
        updatedState.insert("usePerspective", command.isEnabled());
        // It can take a couple frames to properly update icon gizmo positions, so render 3 frames
        renderCount = 3;
        break;
    case View3DActionCommand::OrientationToggle:
        updatedState.insert("globalOrientation", command.isEnabled());
        break;
    case View3DActionCommand::EditLightToggle:
        updatedState.insert("showEditLight", command.isEnabled());
        break;
    case View3DActionCommand::ShowGrid:
        updatedState.insert("showGrid", command.isEnabled());
        break;
    default:
        break;
    }

    if (!updatedState.isEmpty()) {
        QMetaObject::invokeMethod(m_editView3DRootItem,
                                  "updateToolStates",
                                  Q_ARG(QVariant, updatedState),
                                  Q_ARG(QVariant, QVariant::fromValue(false)));
    }

    render3DEditView(renderCount);
}

void Quick3dNodeInstanceServer::requestModelNodePreviewImage(
    const RequestModelNodePreviewImageCommand &command)
{
    m_modelNodePreviewImageCommand = command;
    renderModelNodeImageView();
}

void Quick3dNodeInstanceServer::changeAuxiliaryValues(const ChangeAuxiliaryCommand &command)
{
    Qt5NodeInstanceServer::changeAuxiliaryValues(command);
    render3DEditView();
}

void Quick3dNodeInstanceServer::changePropertyBindings(const ChangeBindingsCommand &command)
{
    Qt5NodeInstanceServer::changePropertyBindings(command);
    render3DEditView();
}

void Quick3dNodeInstanceServer::changeIds(const ChangeIdsCommand &command)
{
    Qt5NodeInstanceServer::changeIds(command);

#ifdef QUICK3D_MODULE
    if (m_editView3DSetupDone) {
        ServerNodeInstance sceneInstance = active3DSceneInstance();
        if (m_active3DSceneUpdatePending) {
            const QString sceneId = sceneInstance.id();
            if (!sceneId.isEmpty())
                updateActiveSceneToEditView3D();
        } else {
            qint32 sceneInstanceId = sceneInstance.instanceId();
            for (const auto &id : command.ids) {
                if (sceneInstanceId == id.instanceId()) {
                    QMetaObject::invokeMethod(m_editView3DRootItem,
                                              "handleActiveSceneIdChange",
                                              Qt::QueuedConnection,
                                              Q_ARG(QVariant, QVariant(sceneInstance.id())));
                    render3DEditView();
                    break;
                }
            }
        }
    }
#endif
}

void Quick3dNodeInstanceServer::changeState(const ChangeStateCommand &command)
{
    Qt5NodeInstanceServer::changeState(command);

    render3DEditView();
}

void Quick3dNodeInstanceServer::removeProperties(const RemovePropertiesCommand &command)
{
    Qt5NodeInstanceServer::removeProperties(command);

    render3DEditView();
}

// update 3D view size when it changes in creator side
void Quick3dNodeInstanceServer::update3DViewState(const Update3dViewStateCommand &command)
{
#ifdef QUICK3D_MODULE
    if (command.type() == Update3dViewStateCommand::SizeChange) {
        if (m_editView3DSetupDone) {
            m_editView3DRootItem->setSize(command.size());
            auto helper = qobject_cast<QmlDesigner::Internal::GeneralHelper *>(m_3dHelper);
            if (helper)
                helper->storeToolState(helper->globalStateId(),
                                       helper->rootSizeKey(),
                                       QVariant(command.size()),
                                       0);
            // Queue two renders to make sure icon gizmos update properly
            render3DEditView(2);
        }
    }
#else
    Q_UNUSED(command)
#endif
}

} // namespace QmlDesigner
