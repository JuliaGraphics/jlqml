#ifndef QML_JLQML_H
#define QML_JLQML_H

#include <jlcxx/functions.hpp>
#include <QMetaType>

namespace qmlwrap
{

// Helper to store a Julia value of type Any in a GC-safe way
struct QVariantAny
{
  QVariantAny(jl_value_t* v);
  ~QVariantAny();
  jl_value_t* value;
};

using qvariant_any_t = std::shared_ptr<QVariantAny>;

}

Q_DECLARE_METATYPE(qmlwrap::qvariant_any_t)

#endif
