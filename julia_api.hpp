#ifndef QML_JULIA_API_H
#define QML_JULIA_API_H

#include <vector>

#include <QJSValue>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QVariant>

#include "julia_function.hpp"
#include "julia_signals.hpp"

namespace qmlwrap
{

/// Global API, allowing to call Julia functions from QML
class JuliaAPI : public QQmlPropertyMap
{
  Q_OBJECT
public:

  JuliaSignals* juliaSignals() const
  {
    return m_julia_signals;
  }

  void setJuliaSignals(JuliaSignals* julia_signals);

  void set_js_engine(QJSEngine* e);

  void register_function(const QString& name, jl_value_t* f);

  ~JuliaAPI();

private:
  JuliaSignals* m_julia_signals = nullptr;
  void register_function_internal(JuliaFunction* f);
  QJSEngine* m_engine = nullptr;
  std::vector<JuliaFunction*> m_registered_functions;
};

struct JuliaSingleton
{
  Q_GADGET
  QML_FOREIGN(JuliaAPI)
  QML_SINGLETON
  QML_NAMED_ELEMENT(Julia)
public:

  inline static JuliaAPI *s_singletonInstance = nullptr;

  static JuliaAPI* create(QQmlEngine *, QJSEngine *engine);

private:
  inline static QJSEngine *s_engine = nullptr;
};

} // namespace qmlwrap

#endif
