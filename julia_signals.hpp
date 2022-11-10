#ifndef QML_JULIA_SIGNALS_H
#define QML_JULIA_SIGNALS_H

#include "jlcxx/jlcxx.hpp"

#include <QQuickItem>

namespace qmlwrap
{

/// Groups signals (defined using QML) that can be emitted from Julia
class JuliaSignals : public QQuickItem
{
  Q_OBJECT
  QML_ELEMENT
public:
  JuliaSignals(QQuickItem *parent = nullptr);
  virtual ~JuliaSignals();
  // Emit the signal with the given name
public slots:
  void emit_signal(const char* signal_name, const QVariantList& args);
};

} // namespace qmlwrap

#endif
