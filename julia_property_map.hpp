#ifndef QML_JULIA_PROPERTY_MAP_H
#define QML_JULIA_PROPERTY_MAP_H

#include "jlcxx/jlcxx.hpp"

#include <QQmlPropertyMap>

namespace qmlwrap
{

/// Multimedia display for Julia
class JuliaPropertyMap : public QQmlPropertyMap
{

public:
  JuliaPropertyMap(QObject* parent = nullptr);
  virtual ~JuliaPropertyMap();
  jl_value_t* julia_value() { return m_julia_value; }
  void set_julia_value(jl_value_t* val);

private:
  
  // This corresponds to the Julia object, which itself holds this JuliaPropertyMap together with a Dict
  jl_value_t* m_julia_value = nullptr;

};

} // namespace qmlwrap

#endif
