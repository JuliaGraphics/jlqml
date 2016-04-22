#ifndef QML_JULIA_API_H
#define QML_JULIA_API_H

#include <vector>

#include <QJSValue>
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

  void set_js_engine(QJSEngine* e);
  void set_julia_js_root(QJSValue root)
  {
    m_julia_js_root = root;
  }

  void register_function(const QString& name);

  static JuliaAPI* instance();

  ~JuliaAPI();

public slots:
  void on_about_to_quit();

private:
  JuliaSignals* m_julia_signals = nullptr;
  void register_function_internal(const QString& fname);
  QJSEngine* m_engine = nullptr;
  // This is the root js object, accessible as Julia in QML
  QJSValue m_julia_js_root;
  JuliaAPI() {}
  std::vector<QString> m_registered_functions;
};

QJSValue julia_js_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine);

} // namespace qmlwrap

#endif
