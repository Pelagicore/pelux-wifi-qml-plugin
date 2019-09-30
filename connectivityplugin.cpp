#include "connectivityplugin.h"

#include <QStringList>

QT_BEGIN_NAMESPACE


ConnectivityPlugin::ConnectivityPlugin(QObject *parent) : 
    QObject(parent)
    , m_backend(new WiFiBackend)
{
}

QStringList ConnectivityPlugin::interfaces() const
{
    QStringList list;
    list << Connectivity_WiFi_iid;
    return list;
}

QIviFeatureInterface *ConnectivityPlugin::interfaceInstance(const QString &interface) const
{
    if (interface == Connectivity_WiFi_iid)
        return m_backend;

    return nullptr;
}

QT_END_NAMESPACE
