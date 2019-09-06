#ifndef QML_JULIA_SIGNALS_H
#define QML_JULIA_SIGNALS_H

#include "jlcxx/jlcxx.hpp"

#include <QObject>

namespace qmlwrap
{

/// Groups signals (defined using QML) that can be emitted from Julia
class JuliaSignals : public QObject
{
  Q_OBJECT
public:
  JuliaSignals(QObject* parent = 0);
  virtual ~JuliaSignals();
  // Emit the signal with the given name
public slots:
  void emit_signal(const char* signal_name, const QVariantList& args);
};

} // namespace qmlwrap

#endif
