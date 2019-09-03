#include <QDebug>
#include <QMetaObject>
#include <QQmlEngine>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include "julia_api.hpp"
#include "julia_signals.hpp"
#include "type_conversion.hpp"

// Q_ARG is a macro, so we use one too
#define MAKE_Q_ARG(cpptype) \
if(jl_types_equal(jl_typeof(v), (jl_value_t*)jlcxx::julia_type<cpptype>())) \
{ \
  auto ptr = std::make_shared<argument_wrapper_impl<cpptype>>(); \
  wrappers.push_back(ptr); \
  ptr->value = jlcxx::unbox<cpptype>(v); \
  return Q_ARG(cpptype, ptr->value); \
}

namespace qmlwrap
{

namespace detail
{
  // This is needed because QGenericArgument is a weak reference to a value, so the value needs to exist as long as the QGenericArgument instance
  struct argument_wrapper
  {
    virtual ~argument_wrapper() {}
  };

  template<typename T>
  struct argument_wrapper_impl : public argument_wrapper
  {
    T value;
  };

  QGenericArgument make_arg(std::vector<std::shared_ptr<argument_wrapper>>& wrappers, jl_value_t* v)
  {
    // Because Q_ARG is a macro, we have no choice but to enumerate all types here
    MAKE_Q_ARG(bool)
    MAKE_Q_ARG(double)
    MAKE_Q_ARG(int)
    jl_value_t* from_t = jl_typeof(v);
    jl_value_t* to_t = (jl_value_t*)jlcxx::julia_type<QString>();
    if(from_t == to_t || jl_type_morespecific(from_t, to_t))
    {
      auto ptr = std::make_shared<argument_wrapper_impl<QString>>();
      wrappers.push_back(ptr);
      ptr->value = jlcxx::unbox<QString>(v);
      return Q_ARG(QString, ptr->value);
    }

    throw std::runtime_error("Failed to convert signal argument of type " + jlcxx::julia_type_name((jl_datatype_t*)jl_typeof(v)));
  }

  template<std::size_t... Is>
  struct ApplyVectorArgs
  {
    void operator()(QObject* o, const char* signal_name, jlcxx::ArrayRef<jl_value_t*> args)
    {
      if(sizeof...(Is) == args.size())
      {
        std::vector<std::shared_ptr<argument_wrapper>> wrappers; wrappers.reserve(sizeof...(Is));
        if(!QMetaObject::invokeMethod(o, signal_name, make_arg(wrappers, args[Is])...))
        {
          throw std::runtime_error("Error emitting or finding signal " + std::string(signal_name));
        }
      }
      else
      {
        ApplyVectorArgs<Is..., sizeof...(Is)>()(o, signal_name, args);
      }
    }
  };

  // end recursion
  template<>
  struct ApplyVectorArgs<0,1,2,3,4,5,6,7,8,9,10>
  {
    void operator()(QObject* o, const char* signal_name, jlcxx::ArrayRef<jl_value_t*> args)
    {
      throw std::runtime_error("Too many arguments for signal " + std::string(signal_name));
    }
  };
}

JuliaSignals::JuliaSignals(QObject* parent) : QObject(parent)
{
  JuliaAPI::instance()->setJuliaSignals(this);
}

JuliaSignals::~JuliaSignals()
{
}

void JuliaSignals::emit_signal(const char* signal_name, jlcxx::ArrayRef<jl_value_t*> args)
{
  detail::ApplyVectorArgs<>()(this, signal_name, args);
}

} // namespace qmlwrap
