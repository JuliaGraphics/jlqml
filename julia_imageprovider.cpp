#include "jlcxx/functions.hpp"

#include "julia_api.hpp"
#include "julia_imageprovider.hpp"
#include "foreign_thread_manager.hpp"
#include <QThread>

namespace qmlwrap
{

JuliaImageProvider::JuliaImageProvider(QQmlImageProviderBase::ImageType type) : QQuickImageProvider(type)
{
}

template<typename ImageT>
inline ImageT process_request(JuliaImageProvider::callback_t f, const QString &id, QSize *size, const QSize &requestedSize, int& m_gc_state)
{
  if(f == nullptr)
  {
    throw std::runtime_error("No callback function set for JuliaImageProvider");
  }
  
  int main_gc_state = JL_GC_STATE_UNSAFE;
  if(!QThread::isMainThread())
  {
    // Set the main thread as GC safe and the image loading thread as unsafe before calling the julia function
    ForeignThreadManager::instance().add_thread(QThread::currentThread());
    ForeignThreadManager::gc_safe_enter(m_gc_state);
    QMetaObject::invokeMethod(&ForeignThreadManager::instance(), [&]{ ForeignThreadManager::gc_safe_enter(main_gc_state); },
                          Qt::BlockingQueuedConnection);
    ForeignThreadManager::gc_safe_leave(m_gc_state);
  }

  ImageResult<ImageT> response = jlcxx::unbox<ImageResult<ImageT>>(f(id, requestedSize.width(), requestedSize.height()));

  if(!QThread::isMainThread())
  {
    // Return to the initial GC state
    ForeignThreadManager::gc_safe_enter(m_gc_state);
    QMetaObject::invokeMethod(&ForeignThreadManager::instance(), [&]{ ForeignThreadManager::gc_safe_leave(main_gc_state); },
                          Qt::BlockingQueuedConnection);
  }

  *size = response.m_size;
  return std::move(response.m_image);
}

QImage JuliaImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
  if(imageType() != QQmlImageProviderBase::Image)
  {
    throw std::runtime_error("JuliaImageProvider is not of type Image");
  }
  return process_request<QImage>(m_callback, id, size, requestedSize, m_gc_state);
}

QPixmap JuliaImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
  if(imageType() != QQmlImageProviderBase::Pixmap)
  {
    throw std::runtime_error("JuliaImageProvider is not of type Pixmap");
  }
  return process_request<QPixmap>(m_callback, id, size, requestedSize, m_gc_state);
}

QQuickTextureFactory *JuliaImageProvider::requestTexture(const QString &id, QSize *size, const QSize &requestedSize)
{
  if(imageType() != QQmlImageProviderBase::Texture)
  {
    throw std::runtime_error("JuliaImageProvider is not of type Texture");
  }
  return process_request<QQuickTextureFactory*>(m_callback, id, size, requestedSize, m_gc_state);
}

void JuliaImageProvider::set_callback(jlcxx::SafeCFunction fdata)
{
  m_callback = jlcxx::make_function_pointer<jl_value_t*(const QString&,int,int)>(fdata);
}

} // namespace qmlwrap

