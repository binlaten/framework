/* * This file is part of meego-im-framework *
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * All rights reserved.
 * Contact: Nokia Corporation (directui@nokia.com)
 *
 * If you have questions regarding the use of this file, please contact
 * Nokia at directui@nokia.com.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * and appearing in the file LICENSE.LGPL included in the packaging
 * of this file.
 */

#include "minputcontextconnection.h"

#include "mabstractinputmethod.h"
#include "mattributeextensionmanager.h"
#include "mattributeextensionid.h"
#include "mimapplication.h" // For the MIMApplication singelton

namespace {
    // attribute names for updateWidgetInformation() map
    const char * const FocusStateAttribute = "focusState";
    const char * const ContentTypeAttribute = "contentType";
    const char * const CorrectionAttribute = "correctionEnabled";
    const char * const PredictionAttribute = "predictionEnabled";
    const char * const AutoCapitalizationAttribute = "autocapitalizationEnabled";
    const char * const SurroundingTextAttribute = "surroundingText";
    const char * const AnchorPositionAttribute = "anchorPosition";
    const char * const CursorPositionAttribute = "cursorPosition";
    const char * const HasSelectionAttribute = "hasSelection";
    const char * const InputMethodModeAttribute = "inputMethodMode";
    const char * const VisualizationAttribute = "visualizationPriority";
    const char * const ToolbarIdAttribute = "toolbarId";
    const char * const ToolbarAttribute = "toolbar";
    const char * const WinId = "winId";
    const char * const CursorRectAttribute = "cursorRectangle";
    const char * const HiddenTextAttribute = "hiddenText";
    const char * const PreeditClickPosAttribute = "preeditClickPos";
}

class MInputContextConnectionPrivate
{
public:
    MInputContextConnectionPrivate();
    ~MInputContextConnectionPrivate();

    QSet<MAbstractInputMethod *> targets; // not owned by us
};


MInputContextConnectionPrivate::MInputContextConnectionPrivate()
{
    // nothing
}


MInputContextConnectionPrivate::~MInputContextConnectionPrivate()
{
    // nothing
}


////////////////////////
// actual class

MInputContextConnection::MInputContextConnection(QObject *parent)
    : d(new MInputContextConnectionPrivate)
{
    Q_UNUSED(parent);

    connect(&MAttributeExtensionManager::instance(), SIGNAL(keyOverrideCreated()),
            this,                                    SIGNAL(keyOverrideCreated()));
}


MInputContextConnection::~MInputContextConnection()
{
    delete d;
}

void MInputContextConnection::addTarget(MAbstractInputMethod *target)
{
    d->targets.insert(target);
    target->handleAppOrientationChanged(lastOrientation);
}

void MInputContextConnection::removeTarget(MAbstractInputMethod *target)
{
    d->targets.remove(target);
}

void MInputContextConnection::updateInputMethodArea(const QRegion &region)
{
    Q_UNUSED(region);
}

QSet<MAbstractInputMethod *> MInputContextConnection::targets()
{
    return d->targets;
}

/* Accessors to widgetState */
int MInputContextConnection::contentType(bool &valid)
{
    QVariant contentTypeVariant = widgetState[ContentTypeAttribute];
    return contentTypeVariant.toInt(&valid);
}

bool MInputContextConnection::correctionEnabled(bool &valid)
{
    QVariant correctionVariant = widgetState[CorrectionAttribute];
    valid = correctionVariant.isValid();
    return correctionVariant.toBool();
}


bool MInputContextConnection::predictionEnabled(bool &valid)
{
    QVariant predictionVariant = widgetState[PredictionAttribute];
    valid = predictionVariant.isValid();
    return predictionVariant.toBool();
}

bool MInputContextConnection::autoCapitalizationEnabled(bool &valid)
{
    QVariant capitalizationVariant = widgetState[AutoCapitalizationAttribute];
    valid = capitalizationVariant.isValid();
    return capitalizationVariant.toBool();
}

QRect MInputContextConnection::cursorRectangle(bool &valid)
{
    QVariant cursorRectVariant = widgetState[CursorRectAttribute];
    valid = cursorRectVariant.isValid();
    return cursorRectVariant.toRect();
}

bool MInputContextConnection::hiddenText(bool &valid)
{
    QVariant hiddenTextVariant = widgetState[HiddenTextAttribute];
    valid = hiddenTextVariant.isValid();
    return hiddenTextVariant.toBool();
}

bool MInputContextConnection::surroundingText(QString &text, int &cursorPosition)
{
    QVariant textVariant = widgetState[SurroundingTextAttribute];
    QVariant posVariant = widgetState[CursorPositionAttribute];

    if (textVariant.isValid() && posVariant.isValid()) {
        text = textVariant.toString();
        cursorPosition = posVariant.toInt();
        return true;
    }

    return false;
}

bool MInputContextConnection::hasSelection(bool &valid)
{
    QVariant selectionVariant = widgetState[HasSelectionAttribute];
    valid = selectionVariant.isValid();
    return selectionVariant.toBool();
}

int MInputContextConnection::inputMethodMode(bool &valid)
{
    QVariant modeVariant = widgetState[InputMethodModeAttribute];
    return modeVariant.toInt(&valid);
}

QRect MInputContextConnection::preeditRectangle(bool &valid)
{
    valid = false;
    return QRect();
}

WId MInputContextConnection::winId(bool &valid)
{
    QVariant winIdVariant = widgetState[WinId];
    // after transfer by dbus type can change
    switch (winIdVariant.type()) {
    case QVariant::UInt:
        valid = (sizeof(uint) >= sizeof(WId));
        return winIdVariant.toUInt();
    case QVariant::ULongLong:
        valid = (sizeof(qulonglong) >= sizeof(WId));
        return winIdVariant.toULongLong();
    default:
        valid = winIdVariant.canConvert<WId>();
    }
    return winIdVariant.value<WId>();
}

int MInputContextConnection::anchorPosition(bool &valid)
{
    QVariant posVariant = widgetState[AnchorPositionAttribute];
    valid = posVariant.isValid();
    return posVariant.toInt();
}

int MInputContextConnection::preeditClickPos(bool &valid) const
{
    QVariant selectionVariant = widgetState[PreeditClickPosAttribute];
    valid = selectionVariant.isValid();
    return selectionVariant.toInt();
}

/* End accessors to widget state */

void MInputContextConnection::updateTransientHint()
{
    bool ok = false;
    WId appWinId = winId(ok);

    if (ok) {
        MIMApplication *app = MIMApplication::instance();

        if (app) {
            app->setTransientHint(appWinId);
        }
    }
}

/* Handlers for inbound communication */
void MInputContextConnection::showInputMethod(unsigned int connectionId)
{
    if (activeConnection != connectionId)
        return;

    Q_EMIT showInputMethodRequest();
}


void MInputContextConnection::hideInputMethod(unsigned int connectionId)
{
    // Only allow this call for current active connection.
    if (activeConnection != connectionId)
        return;

    Q_EMIT hideInputMethodRequest();
}


void MInputContextConnection::mouseClickedOnPreedit(unsigned int connectionId,
                                                            const QPoint &pos, const QRect &preeditRect)
{
    if (activeConnection != connectionId)
        return;

    Q_FOREACH (MAbstractInputMethod *target, targets()) {
        target->handleMouseClickOnPreedit(pos, preeditRect);
    }
}


void MInputContextConnection::setPreedit(unsigned int connectionId,
                                                 const QString &text, int cursorPos)
{
    if (activeConnection != connectionId)
        return;

    preedit = text;

    Q_FOREACH (MAbstractInputMethod *target, targets()) {
        target->setPreedit(text, cursorPos);
    }
}


void MInputContextConnection::reset(unsigned int connectionId)
{
    if (activeConnection != connectionId)
        return;

    preedit.clear();

    Q_FOREACH (MAbstractInputMethod *target, targets()) {
        target->reset();
    }

    if (!preedit.isEmpty()) {
        qWarning("Preedit set from InputMethod::reset()!");
        preedit.clear();
    }
}

void
MInputContextConnection::updateWidgetInformation(
    unsigned int connectionId, const QMap<QString, QVariant> &stateInfo,
    bool focusChanged)
{
    // check visualization change
    bool oldVisualization = false;
    bool newVisualization = false;

    QVariant variant = widgetState[VisualizationAttribute];

    if (variant.isValid()) {
        oldVisualization = variant.toBool();
    }

    variant = stateInfo[VisualizationAttribute];
    if (variant.isValid()) {
        newVisualization = variant.toBool();
    }

    // toolbar change
    MAttributeExtensionId oldAttributeExtensionId;
    MAttributeExtensionId newAttributeExtensionId;
    oldAttributeExtensionId = attributeExtensionId;

    variant = stateInfo[ToolbarIdAttribute];
    if (variant.isValid()) {
        // map toolbar id from local to global
        newAttributeExtensionId = MAttributeExtensionId(variant.toInt(), QString::number(connectionId));
    }
    if (!newAttributeExtensionId.isValid()) {
        newAttributeExtensionId = MAttributeExtensionId::standardAttributeExtensionId();
    }

    // update state
    widgetState = stateInfo;

    if (focusChanged) {
        Q_FOREACH (MAbstractInputMethod *target, targets()) {
            target->handleFocusChange(stateInfo[FocusStateAttribute].toBool());
        }

        updateTransientHint();
    }

    // call notification methods if needed
    if (oldVisualization != newVisualization) {
        Q_FOREACH (MAbstractInputMethod *target, targets()) {
            target->handleVisualizationPriorityChange(newVisualization);
        }
    }

    // compare the toolbar id (global)
    if (oldAttributeExtensionId != newAttributeExtensionId) {
        QString toolbarFile = stateInfo[ToolbarAttribute].toString();
        if (!MAttributeExtensionManager::instance().contains(newAttributeExtensionId) && !toolbarFile.isEmpty()) {
            // register toolbar if toolbar manager does not contain it but
            // toolbar file is not empty. This can reload the toolbar data
            // if im-uiserver crashes.
            variant = stateInfo[ToolbarIdAttribute];
            if (variant.isValid()) {
                const int toolbarLocalId = variant.toInt();
                registerAttributeExtension(connectionId, toolbarLocalId, toolbarFile);
            }
        }
        Q_EMIT toolbarIdChanged(newAttributeExtensionId);
        // store the new used toolbar id(global).
        attributeExtensionId = newAttributeExtensionId;
    }

    // general notification last
    Q_FOREACH (MAbstractInputMethod *target, targets()) {
        target->update();
    }
}

void
MInputContextConnection::receivedAppOrientationAboutToChange(unsigned int connectionId,
                                                                     int angle)
{
    if (activeConnection != connectionId)
        return;

    // Needs to be passed to the MImRotationAnimation listening
    // to this signal first before the plugins. This ensures
    // that the rotation animation can be painted sufficiently early.
    Q_EMIT appOrientationAboutToChange(angle);
    Q_FOREACH (MAbstractInputMethod *target, targets()) {
        target->handleAppOrientationAboutToChange(angle);
    }
}


void MInputContextConnection::receivedAppOrientationChanged(unsigned int connectionId,
                                                                    int angle)
{
    if (activeConnection != connectionId)
        return;

    // Handle orientation changes through MImRotationAnimation with priority.
    // That's needed for getting the correct rotated pixmap buffers.
    Q_EMIT appOrientationChanged(angle);
    Q_FOREACH (MAbstractInputMethod *target, targets()) {
        target->handleAppOrientationChanged(angle);
    }
    lastOrientation = angle;
}


void MInputContextConnection::setCopyPasteState(unsigned int connectionId,
                                                        bool copyAvailable, bool pasteAvailable)
{
    if (activeConnection != connectionId)
        return;

    MAttributeExtensionManager::instance().setCopyPasteState(copyAvailable, pasteAvailable);
}


void MInputContextConnection::processKeyEvent(
    unsigned int connectionId, QEvent::Type keyType, Qt::Key keyCode,
    Qt::KeyboardModifiers modifiers, const QString &text, bool autoRepeat, int count,
    quint32 nativeScanCode, quint32 nativeModifiers, unsigned long time)
{
    if (activeConnection != connectionId)
        return;

    Q_FOREACH (MAbstractInputMethod *target, targets()) {
        target->processKeyEvent(keyType, keyCode, modifiers, text, autoRepeat, count,
                                nativeScanCode, nativeModifiers, time);
    }
}

void MInputContextConnection::registerAttributeExtension(unsigned int connectionId, int id,
                                                         const QString &attributeExtension)
{
    MAttributeExtensionId globalId(id, QString::number(connectionId));
    if (globalId.isValid() && !attributeExtensionIds.contains(globalId)) {
        MAttributeExtensionManager::instance().registerAttributeExtension(globalId, attributeExtension);
        attributeExtensionIds.insert(globalId);
    }
}

void MInputContextConnection::unregisterAttributeExtension(unsigned int connectionId, int id)
{
    MAttributeExtensionId globalId(id, QString::number(connectionId));
    if (globalId.isValid() && attributeExtensionIds.contains(globalId)) {
        MAttributeExtensionManager::instance().unregisterAttributeExtension(globalId);
        attributeExtensionIds.remove(globalId);
    }
}

void MInputContextConnection::setExtendedAttribute(
    unsigned int connectionId, int id, const QString &target, const QString &targetName,
    const QString &attribute, const QVariant &value)
{
    qDebug() << __PRETTY_FUNCTION__;
    MAttributeExtensionId globalId(id, QString::number(connectionId));
    if (globalId.isValid() && attributeExtensionIds.contains(globalId)) {
        MAttributeExtensionManager::instance().setExtendedAttribute(globalId, target, targetName, attribute, value);
    }
}
/* End handlers for inbound communication */

bool MInputContextConnection::detectableAutoRepeat()
{
    return mDetectableAutoRepeat;
}

void MInputContextConnection::setDetectableAutoRepeat(bool enabled)
{
    mDetectableAutoRepeat = enabled;
}

void MInputContextConnection::setGlobalCorrectionEnabled(bool enabled)
{
    mGlobalCorrectionEnabled = enabled;
}

bool MInputContextConnection::globalCorrectionEnabled()
{
    return mGlobalCorrectionEnabled;
}

void MInputContextConnection::setRedirectKeys(bool enabled)
{
    mRedirectionEnabled = enabled;
}

bool MInputContextConnection::redirectKeysEnabled()
{
    return mRedirectionEnabled;
}

/* */
void MInputContextConnection::sendCommitString(const QString &string, int replaceStart,
                                          int replaceLength, int cursorPos) {

    const int cursorPosition(widgetState[CursorPositionAttribute].toInt());
    bool validAnchor(false);

    preedit.clear();

    if (replaceLength == 0  // we don't support replacement
        // we don't support selections
        && anchorPosition(validAnchor) == cursorPosition
        && validAnchor) {
        const int insertPosition(cursorPosition + replaceStart);
        if (insertPosition >= 0) {
            widgetState[SurroundingTextAttribute]
                = widgetState[SurroundingTextAttribute].toString().insert(insertPosition, string);
            widgetState[CursorPositionAttribute] = cursorPos < 0 ? (insertPosition + string.length()) : cursorPos;
            widgetState[AnchorPositionAttribute] = widgetState[CursorPositionAttribute];
        }
    }
}

void MInputContextConnection::sendKeyEvent(const QKeyEvent &keyEvent,
                                           MInputMethod::EventRequestType requestType)
{
    if (requestType != MInputMethod::EventRequestSignalOnly
        && preedit.isEmpty()
        && keyEvent.key() == Qt::Key_Backspace
        && keyEvent.type() == QEvent::KeyPress) {
        QString surrString(widgetState[SurroundingTextAttribute].toString());
        const int cursorPosition(widgetState[CursorPositionAttribute].toInt());
        bool validAnchor(false);

        if (!surrString.isEmpty()
            && cursorPosition > 0
            // we don't support selections
            && anchorPosition(validAnchor) == cursorPosition
            && validAnchor) {
            widgetState[SurroundingTextAttribute] = surrString.remove(cursorPosition - 1, 1);
            widgetState[CursorPositionAttribute] = cursorPosition - 1;
            widgetState[AnchorPositionAttribute] = cursorPosition - 1;
        }
    }
}
/* */

/* */
void MInputContextConnection::handleDisconnection(unsigned int connectionId)
{
    // unregister toolbars registered by the lost connection
    const QString service(QString::number(connectionId));
    QSet<MAttributeExtensionId>::iterator i(attributeExtensionIds.begin());
    while (i != attributeExtensionIds.end()) {
        if ((*i).service() == service) {
            MAttributeExtensionManager::instance().unregisterAttributeExtension(*i);
            i = attributeExtensionIds.erase(i);
        } else {
            ++i;
        }
    }

    if (activeConnection != connectionId) {
        return;
    }

    activeConnection = 0;

    // notify plugins
    Q_FOREACH (MAbstractInputMethod *target, targets()) {
        target->handleClientChange();
    }
}

void MInputContextConnection::activateContext(unsigned int connectionId)
{
    if (connectionId == activeConnection) {
        return;
    }

    /* Notify current/previously active context that it is no longer active */
    sendActivationLostEvent();

    activeConnection = connectionId;

    /* Notify new input context about state/settings stored in the IM server */
    if (activeConnection) {
        /* Hack: Circumvent if(newValue == oldValue) return; guards */
        mGlobalCorrectionEnabled = !mGlobalCorrectionEnabled;
        setGlobalCorrectionEnabled(!mGlobalCorrectionEnabled);

        mRedirectionEnabled = !mRedirectionEnabled;
        setRedirectKeys(!mRedirectionEnabled);

        mDetectableAutoRepeat = !mDetectableAutoRepeat;
        setDetectableAutoRepeat(!mDetectableAutoRepeat);
    }

    // notify plugins
    Q_FOREACH (MAbstractInputMethod *target, targets()) {
        target->handleClientChange();
    }
}
/* */

void MInputContextConnection::sendPreeditString(const QString &string,
                                                const QList<MInputMethod::PreeditTextFormat> &preeditFormats,
                                                int replaceStart, int replaceLength,
                                                int cursorPos)
{
    Q_UNUSED(preeditFormats);
    Q_UNUSED(replaceStart);
    Q_UNUSED(replaceLength);
    Q_UNUSED(cursorPos);
    if (activeConnection) {
        preedit = string;
    }
}

/* */
void MInputContextConnection::setSelection(int start, int length)
{
    Q_UNUSED(start);
    Q_UNUSED(length);
}


void MInputContextConnection::notifyImInitiatedHiding()
{}


void MInputContextConnection::copy()
{}

void MInputContextConnection::paste()
{}


QString MInputContextConnection::selection(bool &valid)
{
    valid = false;
    return QString();
}

void MInputContextConnection::setLanguage(const QString &language)
{
    Q_UNUSED(language);
}

void MInputContextConnection::sendActivationLostEvent()
{}
