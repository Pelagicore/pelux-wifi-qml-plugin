#ifndef CONNECTIVITY_CONNECTIVITYPLUGIN_H_
#define CONNECTIVITY_CONNECTIVITYPLUGIN_H_

#include <QVector>
#include <QtIviCore/QIviServiceInterface>

#include "wifibackend.h"

class ConnectivityPlugin : public QObject, QIviServiceInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QIviServiceInterface_iid FILE "connectivity.json")
    Q_INTERFACES(QIviServiceInterface)

public:
    typedef QVector<QIviFeatureInterface *> (InterfaceBuilder)(ConnectivityPlugin *);

    explicit ConnectivityPlugin(QObject *parent = nullptr);

    QStringList interfaces() const override;
    QIviFeatureInterface* interfaceInstance(const QString& interface) const override;

private:
    QVector<QIviFeatureInterface *> m_interfaces;
    WiFiBackend *m_backend = nullptr;
};


#endif // CONNECTIVITY_CONNECTIVITYPLUGIN_H_
