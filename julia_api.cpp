#include <QDebug>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include "julia_api.hpp"
#include "type_conversion.hpp"

namespace qmlwrap
{

QVariant JuliaAPI::call(const QString& fname, const QVariantList& args)
{
  jl_function_t *func = jl_get_function(jl_current_module, fname.toStdString().c_str());
  if(func == nullptr)
  {
    qWarning() << "Julia method " << fname << " was not found.";
    return QVariant();
  }

  QVariant result_var;

  const int nb_args = args.size();

  jl_value_t* result = nullptr;
  jl_value_t** julia_args;
  JL_GC_PUSH1(&result);
  JL_GC_PUSHARGS(julia_args, nb_args);

  // Process arguments
  for(int i = 0; i != nb_args; ++i)
  {
    julia_args[i] = cxx_wrap::convert_to_julia<QVariant>(args.at(i));
    if(julia_args[i] == nullptr)
    {
      qWarning() << "Julia argument type for function " << fname << " is unsupported:" << args[0].typeName();
      JL_GC_POP();
      JL_GC_POP();
      return QVariant();
    }
  }

  // Do the call
  result = jl_call(func, julia_args, nb_args);
  if (jl_exception_occurred())
  {
    qWarning() << "Exception in Julia callback " << fname << ": " << QString(cxx_wrap::julia_type_name((jl_datatype_t*)jl_typeof(jl_exception_occurred())).c_str());// << ": " << jl_string_data(jl_fieldref(jl_exception_occurred(),0));
    JL_GC_POP();
    JL_GC_POP();
    return QVariant();
  }

  // Process result
  if(result == nullptr)
  {
    qWarning() << "Null result calling Julia function " << fname;
  }
  else if(!jl_is_nothing(result))
  {
    result_var = cxx_wrap::convert_to_cpp<QVariant>(result);
    if(result_var.isNull())
    {
      qWarning() << "Julia method " << fname << " returns unsupported " << QString(cxx_wrap::julia_type_name((jl_datatype_t*)jl_typeof(result)).c_str());
    }
  }
  JL_GC_POP();
  JL_GC_POP();

  return result_var;
}

QVariant JuliaAPI::call(const QString& fname)
{
  return call(fname, QVariantList());
}

void JuliaAPI::setJuliaSignals(JuliaSignals* julia_signals)
{
  m_julia_signals = julia_signals;
}

void JuliaAPI::set_js_engine(QJSEngine* e)
{
  m_engine = e;
  if(m_engine != nullptr)
  {
    for(const QString& fname : m_registered_functions)
    {
      register_function_internal(fname);
    }
    m_registered_functions.clear();
  }
}

void JuliaAPI::register_function(const QString& name)
{
  if(m_engine == nullptr)
  {
    m_registered_functions.push_back(name);
  }
  else
  {
    register_function_internal(name);
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

void JuliaAPI::register_function_internal(const QString& fname)
{
  if(m_engine == nullptr)
  {
    throw std::runtime_error("No JS engine, can't register function");
  }

  QJSValue f = m_engine->evaluate("function() { return Qt.julia.call(\"" + fname + "\", arguments.length === 1 ? [arguments[0]] : Array.apply(null, arguments)); }");

  if(f.isError() || !f.isCallable())
  {
    throw std::runtime_error(("Error setting function" + fname).toStdString());
  }

  m_julia_js_root.setProperty(fname,f);
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
  QJSValue qt_api = engine->globalObject().property("Qt");
  qt_api.setProperty("julia", engine->newQObject(api));
  QQmlEngine::setObjectOwnership(api, QQmlEngine::CppOwnership);
  return result;
}

} // namespace qmlwrap
