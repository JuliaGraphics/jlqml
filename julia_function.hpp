#ifndef QML_JULIA_FUNCTION_H
#define QML_JULIA_FUNCTION_H

#include "jlcxx/jlcxx.hpp"

#include <QJSValue>
#include <QObject>
#include <QQmlEngine>
#include <QVariant>

namespace qmlwrap
{

/// Global API, allowing to call Julia functions from QML
class JuliaFunction : public QObject
{
  Q_OBJECT
public:
  JuliaFunction(const QString& name, jl_function_t* f, QObject* parent);

  // Call a Julia function that takes any number of arguments as a list
  Q_INVOKABLE QVariant call(const QVariantList& arg);

  ~JuliaFunction();

  const QString& name() { return m_name; }

private:
  QString m_name;
  jl_function_t* m_f;
};

} // namespace qmlwrap

#endif
