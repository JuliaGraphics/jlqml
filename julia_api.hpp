#ifndef QML_JULIA_API_H
#define QML_JULIA_API_H

#include <QObject>
#include <QQmlEngine>
#include <QVariant>

namespace qmlwrap
{

/// Global API, allowing to call Julia functions from QML
class JuliaAPI : public QObject
{
  Q_OBJECT
public:

  // Call a Julia function that takes any number of arguments as a list
  Q_INVOKABLE QVariant call(const QString& fname, const QVariantList& arg);

  // Call a Julia function that takes no arguments
  Q_INVOKABLE QVariant call(const QString& fname);
};

QObject* julia_api_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine);

} // namespace qmlwrap

#endif
