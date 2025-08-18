#include <QDebug>

#include "julia_function.hpp"
#include "jlqml.hpp"

namespace qmlwrap
{

jl_module_t* JuliaFunction::m_qml_mod = nullptr;

JuliaFunction::JuliaFunction(const QString& name, jl_value_t* f, QObject* parent) : QObject(parent), m_name(name), m_f(f)
{
  jlcxx::protect_from_gc(m_f);
}

JuliaFunction::~JuliaFunction()
{
  jlcxx::unprotect_from_gc(m_f);
}

QVariant JuliaFunction::call(const QVariantList& args)
{
  using call_julia_func_t = void* (*) (jl_value_t*, const void*);
  static call_julia_func_t call_func = reinterpret_cast<call_julia_func_t>(jlcxx::unbox<void*>(jlcxx::JuliaFunction(jl_get_function(m_qml_mod, "get_julia_call"))()));
  QVariant result_var = *reinterpret_cast<QVariant*>(call_func(m_f, &args));
  return result_var;
}


} // namespace qmlwrap
