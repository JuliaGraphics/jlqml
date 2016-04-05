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
  emit juliaSignalsChanged(julia_signals);
}

QObject* julia_api_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)

  static JuliaAPI* api = new JuliaAPI();
  return api;
}

} // namespace qmlwrap
