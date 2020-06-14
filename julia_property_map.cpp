#include "julia_property_map.hpp"

namespace qmlwrap
{

JuliaPropertyMap::JuliaPropertyMap(QObject *parent) : QQmlPropertyMap(this,parent)
{
}

JuliaPropertyMap::~JuliaPropertyMap()
{
  jlcxx::unprotect_from_gc(m_julia_value);
}

void JuliaPropertyMap::set_julia_value(jl_value_t* val)
{
  jlcxx::protect_from_gc(val);
  m_julia_value = val;
}

} // namespace qmlwrap
