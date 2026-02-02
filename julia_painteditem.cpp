#include "jlcxx/functions.hpp"

#include <QPainter>

#include "julia_api.hpp"
#include "julia_painteditem.hpp"

namespace qmlwrap
{

JuliaPaintedItem::JuliaPaintedItem(QQuickItem *parent) : QQuickPaintedItem(parent)
{
}

void JuliaPaintedItem::paint(QPainter* painter)
{
  m_callback(painter, this);
}

void JuliaPaintedItem::setPaintFunction(jlcxx::SafeCFunction f)
{
  m_callback = jlcxx::make_function_pointer<void(QPainter*,JuliaPaintedItem*)>(f);
}

} // namespace qmlwrap
