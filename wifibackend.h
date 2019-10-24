
#ifndef CONNECTIVITY_WIFIBACKEND_H_
#define CONNECTIVITY_WIFIBACKEND_H_

#include <QObject>
#include <QVariant>
#include <QQmlPropertyMap>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QMap>
#include <QDBusPendingCallWatcher>
#include <QPair>
#include <QTimer>

#include "accesspoint.h"
#include "wifibackendinterface.h"
#include "userinputagent.h"

static const QString connectivityDBusService = "com.luxoft.ConnectivityManager";
static const QString connectivityDBusInterface = "com.luxoft.ConnectivityManager";
static const QString connectivityDBusPath = "/com/luxoft/ConnectivityManager";
static const QString dbusPropertyInterface = "org.freedesktop.DBus.Properties";
static const QString accessPointDBusInterface = "com.luxoft.ConnectivityManager.WiFiAccessPoint";

#define ASYNC_CALL_TIMEOUT 180000 

class WiFiBackend : public WiFiBackendInterface
{
    Q_OBJECT

public:
    explicit WiFiBackend(QObject *parent = nullptr);
    ~WiFiBackend() = default;

    Q_INVOKABLE void initialize() override;

    bool available() const { return m_available; }
    bool enabled() const { return m_enabled; }
    bool hotspotEnabled() const { return m_hotspotEnabled; }
    QString hotspotSSID() const { return m_hotspotSSID; }
    QString hotspotPassword() const { return m_hotspotPassword; }
    ConnectivityModule::ConnectionStatus connectionStatus() const { return m_connectionStatus; }
    AccessPoint activeAccessPoint() const { return m_activeAccessPoint; };
    QString errorString() const { return m_errorString; }

    QVariantList accessPoints() const;
    void setAccessPoints(const QVariantList &accessPoints);
    void setConnectionStatus(ConnectivityModule::ConnectionStatus connectionStatus);
    void setActiveAccessPoint(const AccessPoint &activeAccessPoint);
    void setErrorString(const QString &errorString);

    static QDBusConnection dbusConnection();

public Q_SLOTS:
    void setAvailable(bool available);
    virtual void setEnabled(bool enabled) override;
    virtual void setHotspotEnabled(bool hotspotEnabled);
    virtual void setHotspotSSID(const QString &hotspotSSID);
    virtual void setHotspotPassword(const QString &hotspotPassword);

    virtual QIviPendingReply<void> connectToAccessPoint(const QString &ssid) override;
    virtual QIviPendingReply<void> disconnectFromAccessPoint(const QString &ssid) override;
    virtual QIviPendingReply<void> sendCredentials(const QString &ssid, const QString &password) override;

private Q_SLOTS:
    void propertiesChangedHandler(const QDBusMessage &message);

private:
    void getProperty(const QString &propertyName, std::function<void(QDBusPendingCallWatcher*)> const& lambda);
    bool setProperty(const QString &propertyName, const QVariant &propertyValue);

    void updateAccessPoints();
    void connectSignalsHandler();
    ConnectivityModule::SecurityType securityTypeString2Enum(const QString& securityString);

    bool m_available = false;
    bool m_enabled = false;
    
    bool m_hotspotEnabled = false;
    QString m_hotspotSSID;
    QString m_hotspotPassword;

    ConnectivityModule::ConnectionStatus m_connectionStatus = ConnectivityModule::Disconnected;
    AccessPoint m_activeAccessPoint;
    QString m_activeObjectPath;

    QString m_errorString;

    QMap<QString, QVariant> m_accessPointObjects; //dbus object path -> QVariant(AccessPoint)
    QList<QDBusObjectPath> m_dbusObjList;
    QVariantList m_accessPoints;

    bool m_dbusSignalsConnected = false;

    QObject m_dbusObject;
    UserInputAgent *m_userInputAgent = nullptr;

    void prepareUserInputAgent();
    void destroyUserInputAgent();

    bool m_allowedToUpdateList = true;
    bool m_requiredUpdate = false;

    QTimer m_timerToNotify;
};

#endif // CONNECTIVITY_WIFIBACKEND_H_
