#include "wrap_qml.hpp"

void wrap_part_a(jlcxx::Module& qml_module)
{
  using namespace jlcxx;

  qml_module.add_type<QObject>("QObject")
    .method("deleteLater", &QObject::deleteLater);
  qml_module.method("connect_destroyed_signal", [] (QObject& obj, jl_value_t* jl_f)
  {
    QObject::connect(&obj, &QObject::destroyed, [jl_f](QObject* o)
    {
      static JuliaFunction f(jl_f);
      qmlwrap::GCGuard gc_guard;
      f(o);
    });
  });

  qml_module.add_type<QSize>("QSize")
    .constructor<int,int>()
    .method("width", &QSize::width)
    .method("height", &QSize::height);

  qml_module.add_type<QCoreApplication>("QCoreApplication", julia_base_type<QObject>());
  qml_module.add_type<QGuiApplication>("QGuiApplication", julia_base_type<QCoreApplication>())
    .constructor<int&, char**>();
  qml_module.method("quit", [] () { QGuiApplication::instance()->quit(); });

  qml_module.add_type<QString>("QString", julia_type("AbstractString"))
    .method("cppsize", &QString::size);
  qml_module.method("uint16char", [] (const QString& s, int i) { return static_cast<uint16_t>(s[i].unicode()); });
  qml_module.method("fromStdWString", QString::fromStdWString);
  qml_module.method("isvalidindex", [] (const QString& s, int i)
  {
    if(i < 0 || i >= s.size())
    {
      return false;
    }
    QTextBoundaryFinder bf(QTextBoundaryFinder::Grapheme, s);
    bf.setPosition(i);
    return bf.isAtBoundary();
  });

  qml_module.method("get_iterate", [] (const QString& s, int i)
  {
    if(i < 0 || i >= s.size())
    {
      return std::make_tuple(uint32_t(0),-1);
    }
    QTextBoundaryFinder bf(QTextBoundaryFinder::Grapheme, s);
    bf.setPosition(i);
    if(bf.toNextBoundary() != -1)
    {
      const int nexti = bf.position();
      if((nexti - i) == 1)
      {
        return std::make_tuple(uint32_t(s[i].unicode()), nexti);
      }
      return std::make_tuple(uint32_t(QChar::surrogateToUcs4(s[i],s[i+1])),nexti);
    }
    return std::make_tuple(uint32_t(0),-1);
  });

  qml_module.add_type<qmlwrap::JuliaCanvas>("JuliaCanvas");

  qml_module.add_type<qmlwrap::JuliaDisplay>("JuliaDisplay", julia_type("AbstractDisplay", "Base"))
    .method("load_png", &qmlwrap::JuliaDisplay::load_png)
    .method("load_svg", &qmlwrap::JuliaDisplay::load_svg);

  qml_module.add_type<QUrl>("QUrl")
    .constructor<QString>()
    .method("toString", [] (const QUrl& url) { return url.toString(); })
    .method("toLocalFile", &QUrl::toLocalFile);
  qml_module.method("QUrlFromLocalFile", QUrl::fromLocalFile);

  auto qvar_type = qml_module.add_type<QVariant>("QVariant");
  qvar_type.method("toString", &QVariant::toString);

  qml_module.add_type<QByteArray>("QByteArray").constructor<const char*>()
    .method("to_string", &QByteArray::toStdString);

  qml_module.add_type<QByteArrayView>("QByteArrayView");

  qml_module.add_type<Parametric<TypeVar<1>>>("QList", julia_type("AbstractVector"))
    .apply<QVariantList, QList<QString>, QList<QUrl>, QList<QByteArray>, QList<int>, QList<QObject*>>(qmlwrap::WrapQList());

  // QMap (= QVariantMap for the given type)
  qml_module.add_type<Parametric<TypeVar<1>,TypeVar<2>>>("QMapIterator")
    .apply<qmlwrap::QMapIteratorWrapper<QString, QVariant>>(qmlwrap::WrapQtIterator());
  qml_module.add_type<Parametric<TypeVar<1>,TypeVar<2>>>("QMap", julia_type("AbstractDict"))
    .apply<QMap<QString, QVariant>>(qmlwrap::WrapQtAssociativeContainer<qmlwrap::QMapIteratorWrapper>());

  // QHash
  qml_module.add_type<Parametric<TypeVar<1>,TypeVar<2>>>("QHashIterator")
    .apply<qmlwrap::QHashIteratorWrapper<int, QByteArray>>(qmlwrap::WrapQtIterator());
  qml_module.add_type<Parametric<TypeVar<1>,TypeVar<2>>>("QHash", julia_type("AbstractDict"))
    .apply<QHash<int, QByteArray>>(qmlwrap::WrapQtAssociativeContainer<qmlwrap::QHashIteratorWrapper>());

  qml_module.add_type<QQmlPropertyMap>("QQmlPropertyMap", julia_base_type<QObject>())
    .constructor<QObject *>(jlcxx::finalize_policy::no)
    .method("clear", &QQmlPropertyMap::clear)
    .method("contains", &QQmlPropertyMap::contains)
    .method("insert", static_cast<void(QQmlPropertyMap::*)(const QString&, const QVariant&)>(&QQmlPropertyMap::insert))
    .method("size", &QQmlPropertyMap::size)
    .method("value", &QQmlPropertyMap::value)
    .method("connect_value_changed", [] (QQmlPropertyMap& propmap, jl_value_t* julia_property_map, jl_value_t* callback)
    {
      auto conn = QObject::connect(&propmap, &QQmlPropertyMap::valueChanged, [=](const QString& key, const QVariant& newvalue)
      {
        const jlcxx::JuliaFunction on_value_changed(callback);
        jl_value_t* julia_propmap = julia_property_map;
        qmlwrap::GCGuard gc_guard;
        on_value_changed(julia_propmap, key, newvalue);
      });
    });
  qml_module.add_type<qmlwrap::JuliaPropertyMap>("_JuliaPropertyMap", julia_base_type<QQmlPropertyMap>())
    .method("julia_value", &qmlwrap::JuliaPropertyMap::julia_value)
    .method("set_julia_value", &qmlwrap::JuliaPropertyMap::set_julia_value);

  jlcxx::for_each_parameter_type<qmlwrap::qvariant_types>(qmlwrap::WrapQVariant(qvar_type));
  qml_module.method("type", qmlwrap::julia_variant_type);

  qml_module.method("make_qvariant_map", [] ()
  {
    QVariantMap m;
    m[QString("test")] = QVariant::fromValue(5);
    return QVariant::fromValue(m);
  });
}
