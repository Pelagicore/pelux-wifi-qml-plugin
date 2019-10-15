#include "userinputagent.h"

#include <QDebug>
#include "wifibackend.h"
#include <QThread>
#include <QDBusConnection>
#include <QDBusArgument>


UserInputAgent::UserInputAgent(QObject *parent) : QDBusAbstractAdaptor(parent)
{
}


QMap<QString, QVariant> UserInputAgent::RequestCredentials(const QString &description_type, const QString &description_id, const QMap<QString, QVariant> &requested, const QDBusMessage &message)
{
    QMap<QString, QVariant> response;

    emit credentialsRequested(description_id);

    m_requestData->description_type = description_type;
    m_requestData->description_id = description_id;
    m_requestData->request = requested;

    const QDBusArgument &arg = (requested.value("password")).value<QDBusArgument>();
    arg.beginStructure();
    arg >> m_securityKeyword;
    arg.endStructure();

    message.setDelayedReply(true);
    m_requestData->reply = message.createReply();
    m_requestData->errorReply = message.createErrorReply(QDBusError::Failed, "The user cancelled connection");

    return response;
}

    
void UserInputAgent::sendCredentials(const QString &ssid, const QString &username, const QString &password)
{
    m_requestData->request["ssid"] = QVariant(ssid);
    m_requestData->request["username"] = QVariant(username);

    QDBusArgument arg;
    arg.beginStructure();
    arg << m_securityKeyword << password;
    arg.endStructure();
    m_requestData->request["password"] = QVariant::fromValue(arg);
    m_requestData->reply << m_requestData->request;
    WiFiBackend::dbusConnection().send(m_requestData->reply);
}


void UserInputAgent::cancel()
{
    WiFiBackend::dbusConnection().send(m_requestData->errorReply);
}

