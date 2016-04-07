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
if(jl_types_equal(jl_typeof(v), (jl_value_t*)cxx_wrap::julia_type<cpptype>())) \
{ \
  auto ptr = std::make_shared<argument_wrapper_impl<cpptype>>(); \
  wrappers.push_back(ptr); \
  ptr->value = cxx_wrap::convert_to_cpp<cpptype>(v); \
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
    if(jl_type_morespecific(jl_typeof(v), (jl_value_t*)cxx_wrap::julia_type<QString>()))
    {
      auto ptr = std::make_shared<argument_wrapper_impl<QString>>();
      wrappers.push_back(ptr);
      ptr->value = cxx_wrap::convert_to_cpp<QString>(v);
      return Q_ARG(QString, ptr->value);
    }

    throw std::runtime_error("Failed to convert signal argument of type " + cxx_wrap::julia_type_name((jl_datatype_t*)jl_typeof(v)));
  }

  template<std::size_t... Is>
  struct ApplyVectorArgs
  {
    void operator()(QObject* o, const char* signal_name, cxx_wrap::ArrayRef<jl_value_t*> args)
    {
      if(sizeof...(Is) == jl_array_len(args.wrapped()))
      {
        std::vector<std::shared_ptr<argument_wrapper>> wrappers; wrappers.reserve(sizeof...(Is));
        if(!QMetaObject::invokeMethod(o, signal_name, make_arg(wrappers, *(args.begin()+Is))...))
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
    void operator()(QObject* o, const char* signal_name, cxx_wrap::ArrayRef<jl_value_t*> args)
    {
      throw std::runtime_error("Too many arguments for signal " + std::string(signal_name));
    }
  };
}

JuliaSignals::JuliaSignals(QObject* parent) : QObject(parent)
{
  JuliaAPI* api = qobject_cast<JuliaAPI*>(julia_api_singletontype_provider(nullptr, nullptr));
  api->setJuliaSignals(this);
}

JuliaSignals::~JuliaSignals()
{
}

void JuliaSignals::emit_signal(const char* signal_name, cxx_wrap::ArrayRef<jl_value_t*> args)
{
  detail::ApplyVectorArgs<>()(this, signal_name, args);
}

} // namespace qmlwrap
