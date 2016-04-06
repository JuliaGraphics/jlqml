#include <QDebug>
#include <QMetaObject>
#include <QQmlEngine>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include "julia_api.hpp"
#include "julia_signals.hpp"
#include "type_conversion.hpp"

namespace qmlwrap
{

JuliaSignals::JuliaSignals(QQuickItem* parent) : QQuickItem(parent)
{
}

JuliaSignals::~JuliaSignals()
{
}

void JuliaSignals::emit_signal(const char* signal_name, const cxx_wrap::ArrayRef<jl_value_t*>& args)
{
  if(!QMetaObject::invokeMethod(this, signal_name))
  {
    throw std::runtime_error("No signal named " + std::string(signal_name));
  }
}

void JuliaSignals::componentComplete()
{
  QQuickItem::componentComplete();
  JuliaAPI* api = qobject_cast<JuliaAPI*>(julia_api_singletontype_provider(nullptr, nullptr));
  api->setJuliaSignals(this);
}

} // namespace qmlwrap
