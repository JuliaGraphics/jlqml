#ifndef QML_JULIA_OBJECT_H
#define QML_JULIA_OBJECT_H

#include <string>
#include <map>

#include <QObject>
#include <QQmlPropertyMap>

#include "type_conversion.hpp"

namespace qmlwrap
{

/// Wrap Julia composite types
class JuliaObject : public QQmlPropertyMap
{
  Q_OBJECT
public:
  JuliaObject(jl_value_t* julia_object, QObject* parent = 0);
  virtual ~JuliaObject();
  /// Update a value. Updating a non-existant key is an error.
  void set(const QString& key, const QVariant& value);
  jl_value_t* julia_value() const { return m_julia_object; }

  /// Return the Julia string representation of the stored value
  Q_INVOKABLE QString julia_string() const;

  /// Update the properties in the map from the values in the referred Julia object
  Q_INVOKABLE void update();
private slots:
  void onValueChanged(const QString &key, const QVariant &value);
protected:
  virtual QVariant updateValue(const QString& key, const QVariant& input);
private:
  jl_value_t* m_julia_object;
  std::map<std::string, uint32_t> m_field_mapping;
};

}

#endif
