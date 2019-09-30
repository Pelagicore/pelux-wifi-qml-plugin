#include "wifibackend.h"

#include <QDBusInterface>
#include <QDBusArgument>
#include <QDBusPendingReply>
#include <QDBusPendingCall>

#include <QDebug>

#include "connectivitymodule.h"

WiFiBackend::WiFiBackend(QObject *parent) : WiFiBackendInterface(parent)
{
    qRegisterMetaType<QQmlPropertyMap*>();

    ConnectivityModule::registerTypes();
}


void WiFiBackend::initialize()
{
    m_errorString = "";
    emit errorStringChanged(m_errorString);

    m_available = getProperty("WiFiAvailable").toBool();
    emit availableChanged(m_available);

    m_enabled = getProperty("WiFiEnabled").toBool();
    emit enabledChanged(m_enabled);

    m_hotspotEnabled = getProperty("WiFiHotspotEnabled").toBool();
    emit hotspotEnabledChanged(m_hotspotEnabled);

    m_hotspotSSID = getProperty("WiFiHotspotSSID").toString();
    emit hotspotSSIDChanged(m_hotspotSSID);

    m_hotspotPassword = getProperty("WiFiHotspotPassphrase").toString();
    emit hotspotPasswordChanged(m_hotspotPassword);

    updateAccessPoints();
    connectSignalsHandler();

    emit connectionStatusChanged(m_connectionStatus);
    emit activeAccessPointChanged(m_activeAccessPoint);
    
    emit initializationDone();
}


void WiFiBackend::setAvailable(bool available)
{
    if (m_available == available)
        return;
    m_available = available;
    emit availableChanged(m_available);
}

void WiFiBackend::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    if( setProperty("WiFiEnabled", enabled) ) {
        m_enabled = enabled;
        emit enabledChanged(m_enabled);
    }

}

void WiFiBackend::setHotspotEnabled(bool hotspotEnabled)
{
    if (m_hotspotEnabled == hotspotEnabled)
        return;

    if( setProperty("WiFiHotspotEnabled", hotspotEnabled) ) {
        m_hotspotEnabled = hotspotEnabled;
        emit hotspotEnabledChanged(hotspotEnabled);
    }
}

void WiFiBackend::setHotspotSSID(const QString &hotspotSSID)
{
    if (m_hotspotSSID == hotspotSSID)
        return;

    QByteArray ssid = hotspotSSID.toUtf8();
    if ( ssid.at(ssid.length() - 1) != '\0' ) {
        ssid.append('\0');
    }
    if( setProperty("WiFiHotspotSSID", ssid) ) {
        m_hotspotSSID = hotspotSSID;
        emit hotspotSSIDChanged(hotspotSSID);
    }
}

void WiFiBackend::setHotspotPassword(const QString &hotspotPassword)
{
    if (m_hotspotPassword == hotspotPassword)
        return;

    if( setProperty("WiFiHotspotPassphrase", hotspotPassword) ) {
        m_hotspotPassword = hotspotPassword;
        emit hotspotPasswordChanged(hotspotPassword);
    }
}

void WiFiBackend::setAccessPoints(const QVariantList &accessPoints)
{
    if (m_accessPoints == accessPoints)
        return;
    m_accessPoints = accessPoints;
    emit accessPointsChanged(m_accessPoints);
}

void WiFiBackend::setConnectionStatus(ConnectivityModule::ConnectionStatus connectionStatus)
{
    if (m_connectionStatus == connectionStatus)
        return;
    m_connectionStatus = connectionStatus;
    emit connectionStatusChanged(m_connectionStatus);
}

void WiFiBackend::setActiveAccessPoint(const AccessPoint &activeAccessPoint)
{
    if (m_activeAccessPoint == activeAccessPoint)
        return;
    m_activeAccessPoint = activeAccessPoint;
    emit activeAccessPointChanged(m_activeAccessPoint);
}

void WiFiBackend::setErrorString(const QString &errorString)
{
    if (m_errorString == errorString)
        return;
    m_errorString = errorString;
    emit errorStringChanged(m_errorString);
}


void WiFiBackend::pendingCallFinished(QDBusPendingCallWatcher *watcher)
{
    AccessPoint ap;
    QDBusPendingReply<void> reply = *watcher;
    if (reply.isError()) {
        setConnectionStatus(ConnectivityModule::Disconnected);
        QString name = reply.error().name();
        QString message = reply.error().message();
        setErrorString(message);
        setActiveAccessPoint(AccessPoint("", false, 0, ConnectivityModule::SecurityType::NoSecurity));
        qWarning() << name << ":" << message;
    } else {
        if (connectionStatus() == ConnectivityModule::Connecting) {
            setConnectionStatus(ConnectivityModule::Connected);
        } else {
            setActiveAccessPoint(AccessPoint("", false, 0, ConnectivityModule::SecurityType::NoSecurity));
            setConnectionStatus(ConnectivityModule::Disconnected);
        }
    }

    WiFiBackend::dbusConnection().unregisterObject(userInputAgentDBusPath);
    watcher->deleteLater();
}


void WiFiBackend::prepareUserInputAgent()
{
    if (!m_userInputAgent) {
        m_userInputAgent = new UserInputAgent(&m_dbusObject);
        QObject::connect(m_userInputAgent, &UserInputAgent::credentialsRequested, this, [this](const QString &ssid) {
                Q_EMIT WiFiBackend::credentialsRequested(ssid);
                });
    }
    WiFiBackend::dbusConnection().registerObject(userInputAgentDBusPath, userInputAgentDBusInterface, &m_dbusObject);
}

    
void WiFiBackend::destroyUserInputAgent()
{
}


QIviPendingReply<void> WiFiBackend::connectToAccessPoint(const QString &ssid)
{
    QIviPendingReply<void> reply;
    prepareUserInputAgent();

    setActiveAccessPoint(AccessPoint(ssid, false, 0, ConnectivityModule::SecurityType::NoSecurity));

    QDBusMessage messageConnect = QDBusMessage::createMethodCall(connectivityDBusService, connectivityDBusPath, connectivityDBusInterface, "Connect" );

    if ( !m_accessPointObjects.contains(ssid) ) {
        qWarning() << "Unknown SSID" << ssid << "to connect to.";
        reply.setFailed();
        return reply;
    }

    QVariantList args;
    args.append(QVariant::fromValue(m_accessPointObjects[ssid]));
    args.append(QVariant::fromValue(QDBusObjectPath(userInputAgentDBusPath)));
    messageConnect.setArguments(args);

    QDBusPendingCall pendingCall = WiFiBackend::dbusConnection().asyncCall(messageConnect, ASYNC_CALL_TIMEOUT);
    QDBusPendingCallWatcher *pendingCallWatcher = new QDBusPendingCallWatcher(pendingCall, this);
    QObject::connect(pendingCallWatcher, &QDBusPendingCallWatcher::finished, this, &WiFiBackend::pendingCallFinished);
    
    setConnectionStatus(ConnectivityModule::Connecting);

    reply.setSuccess();
    return reply;
}


QIviPendingReply<void> WiFiBackend::disconnectFromAccessPoint(const QString &ssid)
{
    QIviPendingReply<void> reply;
    QDBusMessage messageConnect = QDBusMessage::createMethodCall(connectivityDBusService, connectivityDBusPath, connectivityDBusInterface, "Disconnect" );

    if ( !m_accessPointObjects.contains(ssid) ) {
        qWarning() << "Unknown SSID" << ssid << "to connect to.";
        reply.setFailed();
        return reply;
    }

    QVariantList args;
    args.append(QVariant::fromValue(m_accessPointObjects[ssid]));

    messageConnect.setArguments(args);

    QDBusPendingCall pendingCall = WiFiBackend::dbusConnection().asyncCall(messageConnect, ASYNC_CALL_TIMEOUT);
    QDBusPendingCallWatcher *pendingCallWatcher = new QDBusPendingCallWatcher(pendingCall, this);
    QObject::connect(pendingCallWatcher, &QDBusPendingCallWatcher::finished, this, &WiFiBackend::pendingCallFinished);
    
    setConnectionStatus(ConnectivityModule::Disconnecting);

    reply.setSuccess();
    return reply;
}

QIviPendingReply<void> WiFiBackend::sendCredentials(const QString &ssid, const QString &password)
{
    m_userInputAgent->sendCredentials(ssid, "", password);

    QIviPendingReply<void> reply;
    reply.setSuccess();
    return reply;
}
    

QDBusConnection WiFiBackend::dbusConnection()
{
    return QDBusConnection::systemBus();
}


QVariant WiFiBackend::getProperty(const QString& propertyName)
{
    QDBusInterface dbusInterface(connectivityDBusService, connectivityDBusPath, connectivityDBusInterface,
                                    WiFiBackend::dbusConnection(), this
                                );

    if (!dbusInterface.isValid()) {
        qWarning() << Q_FUNC_INFO << dbusInterface.lastError().message();
        return QVariant();
    }

    return dbusInterface.property(propertyName.toLatin1().data());
}

    
bool WiFiBackend::setProperty(const QString &propertyName, const QVariant &propertyValue)
{
    QDBusInterface dbusInterface(connectivityDBusService, connectivityDBusPath, connectivityDBusInterface,
                                    WiFiBackend::dbusConnection(), this
                                );

    if (!dbusInterface.isValid()) {
        qWarning() << Q_FUNC_INFO << dbusInterface.lastError().message();
        return false;
    }

    return dbusInterface.setProperty(propertyName.toLatin1().data(), propertyValue);
}


void WiFiBackend::updateAccessPoints()
{
    QList<QDBusObjectPath> dbusObjList = qvariant_cast< QList<QDBusObjectPath> >(getProperty("WiFiAccessPoints"));

    m_accessPoints.clear();
    m_accessPointObjects.clear();

    for (int i=0; i<dbusObjList.count(); i++) {
        QString dbusObjPath = dbusObjList.at(i).path();
        QDBusInterface dbusInterface(connectivityDBusService, dbusObjPath, accessPointDBusInterface,
                                        WiFiBackend::dbusConnection(), this
                                    );
        if (!dbusInterface.isValid()) {
            qWarning() << Q_FUNC_INFO << dbusInterface.lastError().message();
            continue;
        }

        QString ssid = dbusInterface.property("SSID").toString();
        bool connected = dbusInterface.property("Connected").toBool();
        int strength = dbusInterface.property("Strength").toInt();
        QString securityString = dbusInterface.property("Security").toString();
        ConnectivityModule::SecurityType security = securityTypeString2Enum(securityString);
        
        if (ssid != "") {
            m_accessPointObjects.insert( ssid, dbusObjList.at(i) );
        }

        AccessPoint ap(ssid, connected, strength, security);
        if (connected) {
            setActiveAccessPoint(ap);
            setConnectionStatus(ConnectivityModule::Connected);
        }

        /* Connect "PropertiesChanged" for exposed access point object on DBus */
        dbusConnection().connect(connectivityDBusService, dbusObjList.at(i).path(), dbusPropertyInterface, 
            QStringLiteral("PropertiesChanged"), this, SLOT(propertiesChangedHandler(QDBusMessage)));

        m_accessPoints.append(QVariant::fromValue(ap));
    }

    emit accessPointsChanged(m_accessPoints);
}

void WiFiBackend::connectSignalsHandler()
{
    if (m_dbusSignalsConnected) {
        return;
    }

    QDBusConnection conn = WiFiBackend::dbusConnection();
    
    m_dbusSignalsConnected = conn.connect(connectivityDBusService, connectivityDBusPath, dbusPropertyInterface, 
            QStringLiteral("PropertiesChanged"), this, SLOT(propertiesChangedHandler(QDBusMessage)));
}


void WiFiBackend::propertiesChangedHandler(const QDBusMessage &message)
{
    const QVariantList arguments = message.arguments();

    if (arguments.value(0) == connectivityDBusInterface) {
        const QDBusArgument& argument1 = arguments.value(1).value<QDBusArgument>();

        argument1.beginMap();
        while (!argument1.atEnd()) {
            argument1.beginMapEntry();

            const QString &name = argument1.asVariant().toString();
            if (name == "WiFiAvailable") {
                setAvailable( getProperty("WiFiAvailable").toBool() );
            } else if (name == "WiFiEnabled") {
                setEnabled( getProperty("WiFiEnabled").toBool() );
            } else if (name == "WiFiAccessPoints") {
                updateAccessPoints();
            }

            argument1.endMapEntry();
        }
        argument1.endMap();
    } else if (arguments.value(0) == accessPointDBusInterface) {
    
        const QList<QDBusObjectPath> &dbusObjList = qvariant_cast< QList<QDBusObjectPath> >(getProperty("WiFiAccessPoints"));

        for (int i=0; i<dbusObjList.count(); i++) {
            QString dbusObjPath = dbusObjList.at(i).path();

            if ( dbusObjPath == message.path() ) {
                const QDBusArgument &argument1 = arguments.value(1).value<QDBusArgument>();

                AccessPoint ap = m_accessPoints[i].value<AccessPoint>();
                argument1.beginMap();
                while (!argument1.atEnd()) {
                    argument1.beginMapEntry();

                    QDBusInterface dbusInterface(connectivityDBusService, dbusObjPath, accessPointDBusInterface,
                            WiFiBackend::dbusConnection(), this);
                    if (!dbusInterface.isValid()) {
                        qWarning() << Q_FUNC_INFO << dbusInterface.lastError().message();
                        continue;
                    }

                    const QString &name = argument1.asVariant().toString();
                    if (name == "SSID") {
                        ap.setSsid( dbusInterface.property("SSID").toString() );
                    } else if (name == "Connected") {
                        ap.setConnected( dbusInterface.property("Connected").toBool() );
                    } else if (name == "Strength") {
                        ap.setStrength( dbusInterface.property("Strength").toInt() );
                    } else if (name == "Security") {
                        QString securityString = dbusInterface.property("Security").toString();
                        ap.setSecurity( securityTypeString2Enum(securityString) );
                    }

                    argument1.endMapEntry();
                }
                argument1.endMap();
                    
                m_accessPoints.replace(i, QVariant::fromValue(ap));
                emit accessPointsChanged(m_accessPoints);

                break;
            }
        }
    }
}


ConnectivityModule::SecurityType WiFiBackend::securityTypeString2Enum(const QString& securityString)
{
    ConnectivityModule::SecurityType security = ConnectivityModule::SecurityType::NoSecurity;
    if (securityString == "wep") {
        security = ConnectivityModule::SecurityType::WEP;
    } else if (securityString == "wpa-psk") {
        security = ConnectivityModule::SecurityType::WPA_PSK;
    } else if (securityString == "wpa-eap") {
        security = ConnectivityModule::SecurityType::WPA_EAP;
    }
    return security;
}

