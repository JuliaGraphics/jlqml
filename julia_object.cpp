#include <QDebug>
#include "julia_object.hpp"

namespace qmlwrap
{

JuliaObject::JuliaObject(jl_value_t* julia_object, QObject* parent) : QQmlPropertyMap(this, parent), m_julia_object(julia_object)
{
  jl_datatype_t* dt = (jl_datatype_t*)jl_typeof(julia_object);
  if(jl_is_structtype(dt))
  {
    const uint32_t nb_fields = jl_datatype_nfields(dt);
    for(uint32_t i = 0; i != nb_fields; ++i)
    {
      const std::string fname = jlcxx::symbol_name(jl_field_name(dt, i));
      jl_value_t* field_val = jl_fieldref(julia_object, i);
      QVariant qt_fd = jlcxx::convert_to_cpp<QVariant>(field_val);
      if(!qt_fd.isNull())
      {
        m_field_mapping[fname] = i;
        insert(fname.c_str(), qt_fd);
      }
      else if(jl_is_structtype(jl_typeof(field_val)))
      {
        insert(fname.c_str(), QVariant::fromValue(new JuliaObject(field_val, this)));
        m_field_mapping[fname] = i;
      }
      else
      {
        qWarning() << "not converting unsupported field " << fname.c_str() << " of type " << jlcxx::julia_type_name((jl_datatype_t*)jl_typeof(field_val)).c_str();
      }
    }
  }
  else
  {
    qWarning() << "Can't wrap a non-composite type in a JuliaObject";
  }

  QObject::connect(this, &JuliaObject::valueChanged, this, &JuliaObject::onValueChanged);
}

JuliaObject::~JuliaObject()
{
}

void JuliaObject::onValueChanged(const QString &key, const QVariant &value)
{
  const auto map_it = m_field_mapping.find(key.toStdString());
  if(map_it == m_field_mapping.end())
  {
    qWarning() << "value change on unmapped field: " << key << ": " << value;
    return;
  }
  jl_value_t* val = jlcxx::convert_to_julia(value);
  JL_GC_PUSH1(&val);
  jl_set_nth_field(m_julia_object, map_it->second, val);
  JL_GC_POP();
}

void JuliaObject::set(const QString& key, const QVariant& value)
{
  if(!this->contains(key))
  {
    throw std::runtime_error("JuliaObject has no key named " + key.toStdString());
  }

  this->insert(key, value);
  emit valueChanged(key, value);
}

} // namespace qmlwrap
