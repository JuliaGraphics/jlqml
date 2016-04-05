#ifndef QML_JULIA_API_H
#define QML_JULIA_API_H

#include <QObject>
#include <QQmlEngine>
#include <QVariant>

namespace qmlwrap
{

class JuliaSignals;

/// Global API, allowing to call Julia functions from QML
class JuliaAPI : public QObject
{
  Q_OBJECT
  Q_PROPERTY(JuliaSignals* juliaSignals READ juliaSignals WRITE setJuliaSignals NOTIFY juliaSignalsChanged)
public:

  // Call a Julia function that takes any number of arguments as a list
  Q_INVOKABLE QVariant call(const QString& fname, const QVariantList& arg);

  // Call a Julia function that takes no arguments
  Q_INVOKABLE QVariant call(const QString& fname);

  JuliaSignals* juliaSignals() const
  {
    return m_julia_signals;
  }

  void setJuliaSignals(JuliaSignals* julia_signals);

signals:
  void juliaSignalsChanged(JuliaSignals* julia_signals);

private:
  JuliaSignals* m_julia_signals = nullptr;
};

QObject* julia_api_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine);

} // namespace qmlwrap

#endif
