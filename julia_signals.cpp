#include <QDebug>
#include <QMetaMethod>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include "julia_api.hpp"
#include "julia_signals.hpp"
#include "type_conversion.hpp"

namespace qmlwrap
{

JuliaSignals::JuliaSignals(QObject* parent) : QObject(parent)
{
  JuliaAPI* api = qobject_cast<JuliaAPI*>(julia_api_singletontype_provider(nullptr, nullptr));
  api->setJuliaSignals(this);
}

JuliaSignals::~JuliaSignals()
{
}

void JuliaSignals::emit_signal(const QString& signal_name, const cxx_wrap::ArrayRef<jl_value_t*>& args)
{
  QQmlProperty prop(this, signal_name, QQmlEngine::contextForObject(this));
  if(!prop.isValid())
  {
    qWarning() << "sig dont exist";
    //throw std::runtime_error("Signal " + signal_name.toStdString() + " does not exist");
  }
  if(!prop.isSignalProperty())
  {
    qWarning() << "this aint a sig";
    //throw std::runtime_error("Property " + signal_name.toStdString() + " is not a signal");
  }

  prop.method().invoke(this, Qt::DirectConnection);
}

} // namespace qmlwrap
