#ifndef QML_JLQML_H
#define QML_JLQML_H

#include <jlcxx/functions.hpp>
#include <QMetaType>

Q_DECLARE_METATYPE(jlcxx::SafeCFunction)
Q_DECLARE_OPAQUE_POINTER(jl_value_t*)
Q_DECLARE_METATYPE(jl_value_t*)

#endif
