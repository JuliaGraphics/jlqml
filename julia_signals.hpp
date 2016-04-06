#ifndef QML_JULIA_SIGNALS_H
#define QML_JULIA_SIGNALS_H

#include <cxx_wrap.hpp>

#include <QObject>
#include <QQuickItem>

namespace qmlwrap
{

/// Groups signals (defined using QML) that can be emitted from Julia
class JuliaSignals : public QQuickItem
{
  Q_OBJECT
public:
  JuliaSignals(QQuickItem* parent = 0);
  virtual ~JuliaSignals();
  // Emit the signal with the given name
public slots:
  void emit_signal(const char* signal_name, const cxx_wrap::ArrayRef<jl_value_t*>& args);
protected:
  virtual void componentComplete();
};

} // namespace qmlwrap

#endif
