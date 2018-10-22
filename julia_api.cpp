#include <sstream>

#include <QDebug>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include "julia_api.hpp"
#include "type_conversion.hpp"

namespace qmlwrap
{

void JuliaAPI::setJuliaSignals(JuliaSignals* julia_signals)
{
  m_julia_signals = julia_signals;
}

void JuliaAPI::set_js_engine(QJSEngine* e)
{
  m_engine = e;
  if(m_engine != nullptr)
  {
    for(JuliaFunction* f : m_registered_functions)
    {
      register_function_internal(f);
    }
    m_registered_functions.clear();
  }
}

void JuliaAPI::register_function(const QString& name, jl_function_t* f)
{
  JuliaFunction* jf = new JuliaFunction(name, f, this);
  if(m_engine == nullptr)
  {
    m_registered_functions.push_back(jf);
  }
  else
  {
    register_function_internal(jf);
  }
}

JuliaAPI* JuliaAPI::instance()
{
  static JuliaAPI m_instance;
  return &m_instance;
}

JuliaAPI::~JuliaAPI()
{
}

void JuliaAPI::on_about_to_quit()
{
  m_engine = nullptr;
  m_julia_signals = nullptr;
  m_julia_js_root = QJSValue();
}

void JuliaAPI::register_function_internal(JuliaFunction* jf)
{
  if(m_engine == nullptr)
  {
    throw std::runtime_error("No JS engine, can't register function");
  }

  QJSValue f = m_engine->evaluate("(function() { return this." + jf->name() + ".julia_function.call(arguments.length === 1 ? [arguments[0]] : Array.apply(null, arguments)); })");

  if(f.isError() || !f.isCallable())
  {
    throw std::runtime_error(("Error setting function" + jf->name()).toStdString());
  }

  f.setProperty("julia_function", m_engine->newQObject(jf));
  m_julia_js_root.setProperty(jf->name(), f);
}

JuliaAPI::JuliaAPI()
{
}

QJSValue julia_js_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
  QJSValue result = scriptEngine->newObject();
  JuliaAPI* api = JuliaAPI::instance();
  api->set_julia_js_root(result);
  api->set_js_engine(engine);
  QQmlEngine::setObjectOwnership(api, QQmlEngine::CppOwnership);
  return result;
}

} // namespace qmlwrap
