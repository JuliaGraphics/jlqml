#include <QDebug>
#include <QMetaObject>
#include <QQmlEngine>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include "julia_api.hpp"
#include "julia_signals.hpp"
// #include "jlqml.hpp"

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

  template<std::size_t... Is>
  struct ApplyVectorArgs
  {
    void operator()(QObject* o, const char* signal_name, const QVariantList& args)
    {
      if(sizeof...(Is) == args.size())
      {
        if(!QMetaObject::invokeMethod(o, signal_name, Q_ARG(QVariant, args[Is])...))
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
    void operator()(QObject* o, const char* signal_name, const QVariantList& args)
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

void JuliaSignals::emit_signal(const char* signal_name, const QVariantList& args)
{
  detail::ApplyVectorArgs<>()(this, signal_name, args);
}

} // namespace qmlwrap
