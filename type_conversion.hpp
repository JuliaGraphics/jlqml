#ifndef QML_TYPE_CONVERSION_H
#define QML_TYPE_CONVERSION_H

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"
#include "jlcxx/tuple.hpp"

#include <QSize>
#include <QString>
#include <QVariant>

Q_DECLARE_METATYPE(jlcxx::SafeCFunction)
Q_DECLARE_OPAQUE_POINTER(jl_value_t*)
Q_DECLARE_METATYPE(jl_value_t*)

namespace qmlwrap
{
// Mirror struct for QVariant
struct JuliaQVariant
{
  jl_value_t *value;
};

}

namespace jlcxx
{

} // namespace jlcxx


#endif
