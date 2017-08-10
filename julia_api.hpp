#ifndef QML_JULIA_API_H
#define QML_JULIA_API_H

#include <vector>

#include <QJSValue>
#include <QObject>
#include <QQmlEngine>
#include <QVariant>

#include "julia_function.hpp"

namespace qmlwrap
{

class JuliaSignals;

/// Global API, allowing to call Julia functions from QML
class JuliaAPI : public QObject
{
  Q_OBJECT
public:

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

  void register_function(const QString& name, jl_function_t* f);

  static JuliaAPI* instance();

  ~JuliaAPI();

public slots:
  void on_about_to_quit();

private:
  JuliaSignals* m_julia_signals = nullptr;
  void register_function_internal(JuliaFunction* f);
  QJSEngine* m_engine = nullptr;
  // This is the root js object, accessible as Julia in QML
  QJSValue m_julia_js_root;
  JuliaAPI();
  std::vector<JuliaFunction*> m_registered_functions;
};

QJSValue julia_js_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine);

} // namespace qmlwrap

#endif
