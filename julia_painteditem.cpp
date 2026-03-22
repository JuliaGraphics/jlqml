#include "jlcxx/functions.hpp"

#include <QPainter>

#include "julia_api.hpp"
#include "julia_painteditem.hpp"
#include "foreign_thread_manager.hpp"

namespace qmlwrap
{

JuliaPaintedItem::JuliaPaintedItem(QQuickItem *parent) : QQuickPaintedItem(parent)
{
}

void JuliaPaintedItem::paint(QPainter* painter)
{
  GCGuard gc_guard;
  m_callback(painter, this);
}

void JuliaPaintedItem::setPaintFunction(jlcxx::SafeCFunction f)
{
  m_callback = jlcxx::make_function_pointer<void(QPainter*,JuliaPaintedItem*)>(f);
}

} // namespace qmlwrap
