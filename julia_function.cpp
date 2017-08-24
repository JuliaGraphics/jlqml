#include <QDebug>

#include "julia_function.hpp"
#include "type_conversion.hpp"

namespace qmlwrap
{

JuliaFunction::JuliaFunction(const QString& name, jl_function_t* f, QObject* parent) : QObject(parent), m_name(name), m_f(f)
{
}

JuliaFunction::~JuliaFunction()
{
}

QVariant JuliaFunction::call(const QVariantList& args)
{
  QVariant result_var;

  const int nb_args = args.size();

  jl_value_t* result = nullptr;
  jl_value_t** julia_args;
  JL_GC_PUSH1(&result);
  JL_GC_PUSHARGS(julia_args, nb_args);

  // Process arguments
  for(int i = 0; i != nb_args; ++i)
  {
    const auto& qt_arg = args.at(i);
    if(!qt_arg.isValid())
    {
      julia_args[i] = jlcxx::box(static_cast<void*>(nullptr));
    }
    else
    {
      julia_args[i] = jlcxx::convert_to_julia(args.at(i)).value;
    }
    if(julia_args[i] == nullptr)
    {
      qWarning() << "Julia argument type for function " << m_name << " is unsupported:" << args[i].typeName();
      JL_GC_POP();
      JL_GC_POP();
      return QVariant();
    }
  }

  // Do the call
  result = jl_call(m_f, julia_args, nb_args);
  if (jl_exception_occurred())
  {
    jl_call2(jl_get_function(jl_base_module, "show"), jl_stderr_obj(), jl_exception_occurred());
    jl_printf(jl_stderr_stream(), "\n");
    JL_GC_POP();
    JL_GC_POP();
    return QVariant();
  }

  // Process result
  if(result == nullptr)
  {
    qWarning() << "Null result calling Julia function " << m_name;
  }
  else if(!jl_is_nothing(result))
  {
    result_var = jlcxx::convert_to_cpp<QVariant>(JuliaQVariant({result}));
    if(result_var.isNull())
    {
      qWarning() << "Julia method " << m_name << " returns unsupported " << QString(jlcxx::julia_type_name((jl_datatype_t*)jl_typeof(result)).c_str());
    }
  }
  JL_GC_POP();
  JL_GC_POP();

  return result_var;
}


} // namespace qmlwrap
