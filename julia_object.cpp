#include <QDebug>
#include "julia_object.hpp"

#include <jlcxx/functions.hpp>

namespace qmlwrap
{

JuliaObject::JuliaObject(jl_value_t* julia_object, QObject* parent) : QQmlPropertyMap(this, parent), m_julia_object(julia_object)
{
  update();  
}

JuliaObject::~JuliaObject()
{
}

void JuliaObject::onValueChanged(const QString &key, const QVariant &value)
{
  static const jlcxx::JuliaFunction convert("convert");
  const auto map_it = m_field_mapping.find(key.toStdString());
  const size_t fd_idx = map_it->second;
  if(map_it == m_field_mapping.end())
  {
    qWarning() << "value change on unmapped field: " << key << ": " << value;
    return;
  }
  jl_value_t* val = jlcxx::convert_to_julia(value);
  bool value_set = false;
  JL_GC_PUSH1(&val);
  jl_datatype_t* ft = (jl_datatype_t*)jl_field_type(jl_typeof(m_julia_object), fd_idx);
  if(ft == (jl_datatype_t*)jl_typeof(val))
  {
    jl_set_nth_field(m_julia_object, fd_idx, val);
  }
  else if(value.userType() == qMetaTypeId<JuliaObject*>())
  {
    jl_set_nth_field(m_julia_object, fd_idx, value.value<JuliaObject*>()->julia_value());
  }
  else
  {
    jl_set_nth_field(m_julia_object, fd_idx, convert(ft,val));
  }
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

QString JuliaObject::julia_string() const
{
  static const jlcxx::JuliaFunction to_string("string");
  return QString(jlcxx::convert_to_cpp<const char*>(to_string(m_julia_object)));
}

void JuliaObject::update()
{
  this->disconnect(this);

  jl_datatype_t* dt = (jl_datatype_t*)jl_typeof(m_julia_object);
  if(jl_is_structtype(dt))
  {
    const uint32_t nb_fields = jl_datatype_nfields(dt);
    for(uint32_t i = 0; i != nb_fields; ++i)
    {
      const std::string fname = jlcxx::symbol_name(jl_field_name(dt, i));
      jl_value_t* field_val = jl_fieldref(m_julia_object, i);
      QVariant qt_fd = jlcxx::convert_to_cpp<QVariant>(field_val);
      if(!qt_fd.isNull() && qt_fd.userType() != qMetaTypeId<jl_value_t*>())
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

QVariant JuliaObject::updateValue(const QString& key, const QVariant& input)
{
  if(!this->contains(key))
  {
    return QVariant();
  }

  if(input.userType() == qMetaTypeId<jl_value_t*>() && this->value(key).userType() == qMetaTypeId<JuliaObject*>())
  {
    return QVariant::fromValue(new JuliaObject(input.value<jl_value_t*>(), this));
  }

  return input;
}

} // namespace qmlwrap
