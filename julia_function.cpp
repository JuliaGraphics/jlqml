#include <QDebug>

#include "julia_function.hpp"
#include "jlqml.hpp"

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
  using call_julia_func_t = void* (*) (jl_value_t*, const void*);
  static call_julia_func_t call_func = reinterpret_cast<call_julia_func_t>(jlcxx::unbox<void*>(jlcxx::JuliaFunction("get_julia_call", "QML")()));
  QVariant result_var = *reinterpret_cast<QVariant*>(call_func(m_f, &args));
  return result_var;
}


} // namespace qmlwrap
