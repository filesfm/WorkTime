#pragma once

#include <QObject>
#include <QQmlContext>
#include <QQmlEngine>
#include <QRect>

// Test doubles for the context properties src/main.cpp sets on the real
// app's QML engine - screenGeometry, popupPlacement and appQuitter - which
// Main.qml expects to exist. Using the real classes here would mean a
// D-Bus call and GNOME-extension file install (PopupPlacement), a
// dependency on the test machine's actual screen layout (ScreenGeometry),
// or ending the whole test process (the real AppQuitter::quit() calls
// QCoreApplication::exit()).

class ScreenGeometryDouble : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    Q_INVOKABLE QRect availableGeometryAt(const QPoint &) const { return {0, 0, 1920, 1080}; }
    Q_INVOKABLE QRect primaryAvailableGeometry() const { return {0, 0, 1920, 1080}; }
};

class PopupPlacementDouble : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    Q_INVOKABLE void requestPlacement() {}
};

class AppQuitterDouble : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int quitCount MEMBER m_quitCount NOTIFY quitCountChanged)

public:
    using QObject::QObject;

    Q_INVOKABLE void quit()
    {
        ++m_quitCount;
        emit quitCountChanged();
    }

signals:
    void quitCountChanged();

private:
    int m_quitCount = 0;
};

// Installs the doubles above as context properties before any QML test
// file loads. qmlEngineAvailable() is QtQuickTest's documented setup-object
// slot for quick_test_main_with_setup(), invoked once the test engine
// exists and before any QML is loaded.
class QuickTestSetup : public QObject
{
    Q_OBJECT

public slots:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        engine->rootContext()->setContextProperty("screenGeometry", &m_screenGeometry);
        engine->rootContext()->setContextProperty("popupPlacement", &m_popupPlacement);
        engine->rootContext()->setContextProperty("appQuitter", &m_appQuitter);
    }

private:
    ScreenGeometryDouble m_screenGeometry;
    PopupPlacementDouble m_popupPlacement;
    AppQuitterDouble m_appQuitter;
};
