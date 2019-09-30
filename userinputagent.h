#ifndef CONNECTIVITY_USERINPUTAGENT_H_
#define CONNECTIVITY_USERINPUTAGENT_H_

#include <QObject>
#include <QVariant>

#include <QDBusMessage>
#include <QDBusAbstractAdaptor>

#include <QSharedPointer>

#define userInputAgentDBusService "com.luxoft.ConnectivityManager"
#define userInputAgentDBusInterface "com.luxoft.ConnectivityManager.UserInputAgent"
#define userInputAgentDBusPath "/"

struct RequestData
{
    QString description_type;
    QString description_id;
    QMap<QString, QVariant> request;
    QDBusMessage reply;
};

class UserInputAgent : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", userInputAgentDBusInterface)

public:
    explicit UserInputAgent(QObject *parent = nullptr);
    ~UserInputAgent() = default;

    void sendCredentials(const QString &ssid, const QString &username, const QString &password);

public Q_SLOTS:
    QVariantMap RequestCredentials(const QString &description_type, 
            const QString &description_id, 
            const QVariantMap &requested, 
            const QDBusMessage &message);

Q_SIGNALS:
    void credentialsRequested(const QString &ssid);
        
private:
    QSharedPointer<RequestData> m_requestData = QSharedPointer<RequestData>::create();

    QString m_securityKeyword;
};

#endif //CONNECTIVITY_USERINPUTAGENT_H_
