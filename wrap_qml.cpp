#include <QFileSystemWatcher>
#include <QGuiApplication>
#include <QLibraryInfo>
#include <QPainter>
#include <QPaintDevice>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QSurfaceFormat>
#include <QtCore/QTextBoundaryFinder>
#include <QTimer>
#include <QtQml>

#include "application_manager.hpp"
#include "julia_api.hpp"
#include "julia_canvas.hpp"
#include "julia_display.hpp"
#include "julia_itemmodel.hpp"
#include "julia_painteditem.hpp"
#include "julia_property_map.hpp"
#include "julia_signals.hpp"
#include "opengl_viewport.hpp"
#include "makie_viewport.hpp"

#include "jlqml.hpp"

#include "jlcxx/stl.hpp"

namespace jlcxx
{

template<> struct SuperType<QQmlApplicationEngine> { using type = QQmlEngine; };
template<> struct SuperType<QQmlContext> { using type = QObject; };
template<> struct SuperType<QQmlEngine> { using type = QObject; };
template<> struct SuperType<QQmlPropertyMap> { using type = QObject; };
template<> struct SuperType<qmlwrap::JuliaPropertyMap> { using type = QQmlPropertyMap; };
template<> struct SuperType<QQuickView> { using type = QQuickWindow; };
template<> struct SuperType<QTimer> { using type = QObject; };
template<> struct SuperType<qmlwrap::JuliaPaintedItem> { using type = QQuickItem; };
template<> struct SuperType<QAbstractItemModel> { using type = QObject; };
template<> struct SuperType<QAbstractTableModel> { using type = QAbstractItemModel; };
template<> struct SuperType<QQuickItem> { using type = QObject; };
template<> struct SuperType<QWindow> { using type = QObject; };
template<> struct SuperType<QQuickWindow> { using type = QWindow; };
template<> struct SuperType<qmlwrap::JuliaItemModel> { using type = QAbstractTableModel; };

}

namespace qmlwrap
{

QVariantAny::QVariantAny(jl_value_t* v) : value(v)
{
  assert(v != nullptr);
  jlcxx::protect_from_gc(value);
}
QVariantAny::~QVariantAny()
{
  jlcxx::unprotect_from_gc(value);
}

using qvariant_types = jlcxx::ParameterList<bool, float, double, int32_t, int64_t, uint32_t, uint64_t, void*, jl_value_t*,
  QString, QUrl, jlcxx::SafeCFunction, QVariantMap, QVariantList, QStringList, QList<QUrl>, JuliaDisplay*, JuliaCanvas*, JuliaPropertyMap*, QObject*>;

inline std::map<int, jl_datatype_t*> g_variant_type_map;

jl_datatype_t* julia_type_from_qt_id(int id)
{
    if(qmlwrap::g_variant_type_map.count(id) == 0)
    {
      qWarning() << "invalid variant type " << QMetaType(id).name();
    }
    assert(qmlwrap::g_variant_type_map.count(id) == 1);
    return qmlwrap::g_variant_type_map[id];
}

jl_datatype_t* julia_variant_type(const QVariant& v)
{
  if(!v.isValid())
  {
    static jl_datatype_t* nothing_type = (jl_datatype_t*)jlcxx::julia_type("Nothing");
    return nothing_type;
  }
  const int usertype = v.userType();
  if(usertype == qMetaTypeId<QJSValue>())
  {
    return julia_variant_type(v.value<QJSValue>().toVariant());
  }
  // Convert to some known, specific type if necessary
  if(v.canConvert<QObject*>())
  {
    QObject* obj = v.value<QObject*>();
    if(obj != nullptr)
    {
      if(qobject_cast<JuliaDisplay*>(obj) != nullptr)
      {
        return jlcxx::julia_base_type<JuliaDisplay*>();
      }
      if(qobject_cast<JuliaCanvas*>(obj) != nullptr)
      {
        return jlcxx::julia_base_type<JuliaCanvas*>();
      }
      if(dynamic_cast<JuliaPropertyMap*>(obj) != nullptr)
      {
        return (jl_datatype_t*)jlcxx::julia_type("JuliaPropertyMap");
      }
    }
  }

  return julia_type_from_qt_id(usertype);
}

template<typename T>
struct ApplyQVariant
{
  void operator()(jlcxx::TypeWrapper<QVariant>& wrapper)
  {
    g_variant_type_map[qMetaTypeId<T>()] = jlcxx::julia_base_type<T>();
    wrapper.module().method("value", [] (jlcxx::SingletonType<T>, const QVariant& v)
    {
      if(v.userType() == qMetaTypeId<QJSValue>())
      {
        return v.value<QJSValue>().toVariant().value<T>();
      }
      return v.value<T>();
    });
    wrapper.module().method("setValue", [] (jlcxx::SingletonType<T>, QVariant& v, T val)
    {
      v.setValue(val);
    });
    wrapper.module().method("QVariant", [] (jlcxx::SingletonType<T>, T val)
    {
      return QVariant::fromValue(val);
    });
  }
};

template<>
struct ApplyQVariant<jl_value_t*>
{
  void operator()(jlcxx::TypeWrapper<QVariant>& wrapper)
  {
    g_variant_type_map[qMetaTypeId<qvariant_any_t>()] = jl_any_type;
    wrapper.module().method("value", [] (jlcxx::SingletonType<jl_value_t*>, const QVariant& v)
    {
      if(v.userType() == qMetaTypeId<QJSValue>())
      {
        return v.value<QJSValue>().toVariant().value<qvariant_any_t>()->value;
      }
      return v.value<qvariant_any_t>()->value;
    });
    wrapper.module().method("setValue", [] (jlcxx::SingletonType<jl_value_t*>, QVariant& v, jl_value_t* val)
    {
      v.setValue(std::make_shared<QVariantAny>(val));
    });
    wrapper.module().method("QVariant", [] (jlcxx::SingletonType<jl_value_t*>, jl_value_t* val)
    {
      return QVariant::fromValue(std::make_shared<QVariantAny>(val));
    });
  }
};

// These are created in QML when passing a JuliaPropertyMap back to Julia by calling a Julia function from QML
template<>
struct ApplyQVariant<JuliaPropertyMap*>
{
  void operator()(jlcxx::TypeWrapper<QVariant>& wrapper)
  {
    wrapper.module().method("getpropertymap", [] (QVariant& v) { return dynamic_cast<JuliaPropertyMap*>(v.value<QObject*>())->julia_value(); });
  }
};

struct WrapQVariant
{
  WrapQVariant(jlcxx::TypeWrapper<QVariant>& w) : m_wrapper(w)
  {
  }

  template<typename T>
  void apply()
  {
    ApplyQVariant<T>()(m_wrapper);
  }

  jlcxx::TypeWrapper<QVariant>& m_wrapper;
};

struct WrapQList
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("cppgetindex", [] (const WrappedT& list, const int i) -> typename WrappedT::const_reference { return list[i]; });
    wrapped.method("cppsetindex!", [] (WrappedT& list, const typename WrappedT::value_type& v, const int i) { list[i] = v; });
    wrapped.method("push_back", static_cast<void(WrappedT::*)(typename WrappedT::parameter_type)>(&WrappedT::push_back));
    wrapped.method("clear", &WrappedT::clear);
    wrapped.method("removeAt", &WrappedT::removeAt);
  }
};

// Wrap a ContainerT<KeyT,ValueT>::iterator in a templated struct so it can be added as a parametric type in Julia
template<template<typename, typename> typename ContainerT, typename KeyT, typename ValueT>
struct QtIteratorWrapper
{
  using iterator_type = typename ContainerT<KeyT, ValueT>::iterator;
  using value_type = ValueT;
  using key_type = KeyT;
  iterator_type value;
};

template<typename KeyT, typename ValueT> struct QHashIteratorWrapper : QtIteratorWrapper<QHash, KeyT, ValueT> {};
template<typename KeyT, typename ValueT> struct QMapIteratorWrapper : QtIteratorWrapper<QMap, KeyT, ValueT> {};

template<typename T>
void validate_iterator(T it)
{
  using IteratorT = typename T::iterator_type;
  if(it.value == IteratorT())
  {
    throw std::runtime_error("Invalid iterator");
  }
}

struct WrapQtIterator
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using KeyT = typename WrappedT::key_type;
    using ValueT = typename WrappedT::value_type;

    wrapped.method("iteratornext", [] (WrappedT it) -> WrappedT { ++(it.value); return it; });
    wrapped.method("iteratorkey", [] (WrappedT it) -> KeyT { validate_iterator(it); return it.value.key();} );
    wrapped.method("iteratorvalue", [] (WrappedT it) -> ValueT& { validate_iterator(it); return it.value.value(); } );
    wrapped.method("iteratorisequal", [] (WrappedT it1, WrappedT it2) -> bool { return it1.value == it2.value; } );
  }
};

template<template<typename, typename> typename IteratorWrapperT>
struct WrapQtAssociativeContainer
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using ValueT = typename WrappedT::mapped_type;
    using KeyT = typename WrappedT::key_type;

    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("cppgetindex", [] (WrappedT& hash, const KeyT& k) -> ValueT& { return hash[k]; });
    wrapped.method("cppsetindex!", [] (WrappedT& hash, const ValueT& v, const KeyT& k) { hash[k] = v; });
    wrapped.method("insert", [] (WrappedT& hash, const KeyT& k, const ValueT& v) { hash.insert(k,v); });
    wrapped.method("clear", &WrappedT::clear);
    wrapped.method("remove", [] (WrappedT& hash, const KeyT& k) -> bool { return hash.remove(k); });
    wrapped.method("empty", &WrappedT::empty);
    wrapped.method("iteratorbegin", [] (WrappedT& hash) { return IteratorWrapperT<KeyT,ValueT>{hash.begin()}; });
    wrapped.method("iteratorend", [] (WrappedT& hash) { return IteratorWrapperT<KeyT,ValueT>{hash.end()}; });
    wrapped.method("keys", [] (const WrappedT& hash) { return hash.keys(); });
    wrapped.method("values", &WrappedT::values);
    wrapped.method("contains", [] (WrappedT& hash, const KeyT& k) -> bool { return hash.contains(k); });
  }
};

}

JLCXX_MODULE define_julia_module(jlcxx::Module& qml_module)
{
  using namespace jlcxx;

  // Set pointers to Julia QML module in the classes that use it
  qmlwrap::JuliaFunction::m_qml_mod = qml_module.julia_module();
  qmlwrap::ApplicationManager::m_qml_mod = qml_module.julia_module();
  qmlwrap::JuliaItemModel::m_qml_mod = qml_module.julia_module();

  qml_module.method("define_julia_module_makie", [](jl_value_t* mod)
  {
    qmlwrap::MakieViewport::m_qmlmakie_mod = reinterpret_cast<jl_module_t*>(mod);
  });

  qml_module.method("set_default_makie_renderfunction", [](jlcxx::SafeCFunction renderFunction)
  {
    qmlwrap::MakieViewport::m_default_render_function = renderFunction;
  });

  // Enums
  qml_module.add_enum<Qt::Orientation>("Orientation",
    std::vector<const char*>({
      "Horizontal",
      "Vertical"
    }),
    std::vector<int>({
      Qt::Horizontal,
      Qt::Vertical
    })
  );
  
  qml_module.add_enum<Qt::ItemDataRole>("ItemDataRole",
    std::vector<const char*>({
      "DisplayRole",
      "DecorationRole",
      "EditRole",
      "ToolTipRole",
      "StatusTipRole",
      "WhatsThisRole",
      "FontRole",
      "TextAlignmentRole",
      "BackgroundRole",
      "ForegroundRole",
      "CheckStateRole",
      "AccessibleTextRole",
      "AccessibleDescriptionRole",
      "SizeHintRole",
      "InitialSortOrderRole",
      "UserRole"
    }),
    std::vector<int>({
      Qt::DisplayRole,
      Qt::DecorationRole,
      Qt::EditRole,
      Qt::ToolTipRole,
      Qt::StatusTipRole,
      Qt::WhatsThisRole,
      Qt::FontRole,
      Qt::TextAlignmentRole,
      Qt::BackgroundRole,
      Qt::ForegroundRole,
      Qt::CheckStateRole,
      Qt::AccessibleTextRole,
      Qt::AccessibleDescriptionRole,
      Qt::SizeHintRole,
      Qt::InitialSortOrderRole,
      Qt::UserRole
    })
  );

  qml_module.add_enum<QSGRendererInterface::GraphicsApi>("GraphicsAPI",
    std::vector<const char*>({
      "Unknown",
      "Software",
      "OpenVG",
      "OpenGL",
      "Direct3D11",
      "Vulkan",
      "Metal",
      "Null"
    }),
    std::vector<int>({
      QSGRendererInterface::Unknown,
      QSGRendererInterface::Software,
      QSGRendererInterface::OpenVG,
      QSGRendererInterface::OpenGL,
      QSGRendererInterface::Direct3D11,
      QSGRendererInterface::Vulkan,
      QSGRendererInterface::Metal,
      QSGRendererInterface::Null
    })
  );

  qml_module.add_enum<Qt::MouseButton>("MouseButton",
    std::vector<const char*>({
      "NoButton",
      "AllButtons",
      "LeftButton",
      "RightButton",
      "MiddleButton",
      "BackButton",
      "ForwardButton",
      "TaskButton",
      "ExtraButton4",
      "ExtraButton5",
      "ExtraButton6",
      "ExtraButton7",
      "ExtraButton8",
      "ExtraButton9",
      "ExtraButton10",
      "ExtraButton11",
      "ExtraButton12",
      "ExtraButton13",
      "ExtraButton14",
      "ExtraButton15",
      "ExtraButton16",
      "ExtraButton17",
      "ExtraButton18",
      "ExtraButton19",
      "ExtraButton20",
      "ExtraButton21",
      "ExtraButton22",
      "ExtraButton23",
      "ExtraButton24"
    }),
    std::vector<int>({
      Qt::NoButton,
      Qt::AllButtons,
      Qt::LeftButton,
      Qt::RightButton,
      Qt::MiddleButton,
      Qt::BackButton,
      Qt::ForwardButton,
      Qt::TaskButton,
      Qt::ExtraButton4,
      Qt::ExtraButton5,
      Qt::ExtraButton6,
      Qt::ExtraButton7,
      Qt::ExtraButton8,
      Qt::ExtraButton9,
      Qt::ExtraButton10,
      Qt::ExtraButton11,
      Qt::ExtraButton12,
      Qt::ExtraButton13,
      Qt::ExtraButton14,
      Qt::ExtraButton15,
      Qt::ExtraButton16,
      Qt::ExtraButton17,
      Qt::ExtraButton18,
      Qt::ExtraButton19,
      Qt::ExtraButton20,
      Qt::ExtraButton21,
      Qt::ExtraButton22,
      Qt::ExtraButton23,
      Qt::ExtraButton24
    })
  );
  
  qml_module.add_enum<Qt::Key>("Key",
    std::vector<const char*>({
      "Key_Escape", "Key_Tab", "Key_Backtab", "Key_Backspace", "Key_Return", "Key_Enter", "Key_Insert", "Key_Delete", "Key_Pause", "Key_Print",
      "Key_SysReq", "Key_Clear", "Key_Home", "Key_End", "Key_Left", "Key_Up", "Key_Right", "Key_Down", "Key_PageUp", "Key_PageDown",
      "Key_Shift", "Key_Control", "Key_Meta", "Key_Alt", "Key_AltGr", "Key_CapsLock", "Key_NumLock", "Key_ScrollLock", "Key_F1", "Key_F2",
      "Key_F3", "Key_F4", "Key_F5", "Key_F6", "Key_F7", "Key_F8", "Key_F9", "Key_F10", "Key_F11", "Key_F12",
      "Key_F13", "Key_F14", "Key_F15", "Key_F16", "Key_F17", "Key_F18", "Key_F19", "Key_F20", "Key_F21", "Key_F22",
      "Key_F23", "Key_F24", "Key_F25", "Key_F26", "Key_F27", "Key_F28", "Key_F29", "Key_F30", "Key_F31", "Key_F32",
      "Key_F33", "Key_F34", "Key_F35", "Key_Super_L", "Key_Super_R", "Key_Menu", "Key_Hyper_L", "Key_Hyper_R", "Key_Help", "Key_Direction_L",
      "Key_Direction_R", "Key_Space", "Key_Exclam", "Key_QuoteDbl", "Key_NumberSign", "Key_Dollar", "Key_Percent", "Key_Ampersand", "Key_Apostrophe", "Key_ParenLeft",
      "Key_ParenRight", "Key_Asterisk", "Key_Plus", "Key_Comma", "Key_Minus", "Key_Period", "Key_Slash", "Key_0", "Key_1", "Key_2",
      "Key_3", "Key_4", "Key_5", "Key_6", "Key_7", "Key_8", "Key_9", "Key_Colon", "Key_Semicolon", "Key_Less",
      "Key_Equal", "Key_Greater", "Key_Question", "Key_At", "Key_A", "Key_B", "Key_C", "Key_D", "Key_E", "Key_F",
      "Key_G", "Key_H", "Key_I", "Key_J", "Key_K", "Key_L", "Key_M", "Key_N", "Key_O", "Key_P",
      "Key_Q", "Key_R", "Key_S", "Key_T", "Key_U", "Key_V", "Key_W", "Key_X", "Key_Y", "Key_Z",
      "Key_BracketLeft", "Key_Backslash", "Key_BracketRight", "Key_AsciiCircum", "Key_Underscore", "Key_QuoteLeft", "Key_BraceLeft", "Key_Bar", "Key_BraceRight", "Key_AsciiTilde",
      "Key_nobreakspace", "Key_exclamdown", "Key_cent", "Key_sterling", "Key_currency", "Key_yen", "Key_brokenbar", "Key_section", "Key_diaeresis", "Key_copyright",
      "Key_ordfeminine", "Key_guillemotleft", "Key_notsign", "Key_hyphen", "Key_registered", "Key_macron", "Key_degree", "Key_plusminus", "Key_twosuperior", "Key_threesuperior",
      "Key_acute", "Key_micro", "Key_paragraph", "Key_periodcentered", "Key_cedilla", "Key_onesuperior", "Key_masculine", "Key_guillemotright", "Key_onequarter", "Key_onehalf",
      "Key_threequarters", "Key_questiondown", "Key_Agrave", "Key_Aacute", "Key_Acircumflex", "Key_Atilde", "Key_Adiaeresis", "Key_Aring", "Key_AE", "Key_Ccedilla",
      "Key_Egrave", "Key_Eacute", "Key_Ecircumflex", "Key_Ediaeresis", "Key_Igrave", "Key_Iacute", "Key_Icircumflex", "Key_Idiaeresis", "Key_ETH", "Key_Ntilde",
      "Key_Ograve", "Key_Oacute", "Key_Ocircumflex", "Key_Otilde", "Key_Odiaeresis", "Key_multiply", "Key_Ooblique", "Key_Ugrave", "Key_Uacute", "Key_Ucircumflex",
      "Key_Udiaeresis", "Key_Yacute", "Key_THORN", "Key_ssharp", "Key_division", "Key_ydiaeresis", "Key_Multi_key", "Key_Codeinput", "Key_SingleCandidate", "Key_MultipleCandidate",
      "Key_PreviousCandidate", "Key_Mode_switch", "Key_Kanji", "Key_Muhenkan", "Key_Henkan", "Key_Romaji", "Key_Hiragana", "Key_Katakana", "Key_Hiragana_Katakana", "Key_Zenkaku",
      "Key_Hankaku", "Key_Zenkaku_Hankaku", "Key_Touroku", "Key_Massyo", "Key_Kana_Lock", "Key_Kana_Shift", "Key_Eisu_Shift", "Key_Eisu_toggle", "Key_Hangul", "Key_Hangul_Start",
      "Key_Hangul_End", "Key_Hangul_Hanja", "Key_Hangul_Jamo", "Key_Hangul_Romaja", "Key_Hangul_Jeonja", "Key_Hangul_Banja", "Key_Hangul_PreHanja", "Key_Hangul_PostHanja", "Key_Hangul_Special", "Key_Dead_Grave",
      "Key_Dead_Acute", "Key_Dead_Circumflex", "Key_Dead_Tilde", "Key_Dead_Macron", "Key_Dead_Breve", "Key_Dead_Abovedot", "Key_Dead_Diaeresis", "Key_Dead_Abovering", "Key_Dead_Doubleacute", "Key_Dead_Caron",
      "Key_Dead_Cedilla", "Key_Dead_Ogonek", "Key_Dead_Iota", "Key_Dead_Voiced_Sound", "Key_Dead_Semivoiced_Sound", "Key_Dead_Belowdot", "Key_Dead_Hook", "Key_Dead_Horn", "Key_Dead_Stroke", "Key_Dead_Abovecomma",
      "Key_Dead_Abovereversedcomma", "Key_Dead_Doublegrave", "Key_Dead_Belowring", "Key_Dead_Belowmacron", "Key_Dead_Belowcircumflex", "Key_Dead_Belowtilde", "Key_Dead_Belowbreve", "Key_Dead_Belowdiaeresis", "Key_Dead_Invertedbreve", "Key_Dead_Belowcomma",
      "Key_Dead_Currency", "Key_Dead_a", "Key_Dead_A", "Key_Dead_e", "Key_Dead_E", "Key_Dead_i", "Key_Dead_I", "Key_Dead_o", "Key_Dead_O", "Key_Dead_u",
      "Key_Dead_U", "Key_Dead_Small_Schwa", "Key_Dead_Capital_Schwa", "Key_Dead_Greek", "Key_Dead_Lowline", "Key_Dead_Aboveverticalline", "Key_Dead_Belowverticalline", "Key_Dead_Longsolidusoverlay", "Key_Back", "Key_Forward",
      "Key_Stop", "Key_Refresh", "Key_VolumeDown", "Key_VolumeMute", "Key_VolumeUp", "Key_BassBoost", "Key_BassUp", "Key_BassDown", "Key_TrebleUp", "Key_TrebleDown",
      "Key_MediaPlay", "Key_MediaStop", "Key_MediaPrevious", "Key_MediaNext", "Key_MediaRecord", "Key_MediaPause", "Key_MediaTogglePlayPause", "Key_HomePage", "Key_Favorites", "Key_Search",
      "Key_Standby", "Key_OpenUrl", "Key_LaunchMail", "Key_LaunchMedia", "Key_Launch0", "Key_Launch1", "Key_Launch2", "Key_Launch3", "Key_Launch4", "Key_Launch5",
      "Key_Launch6", "Key_Launch7", "Key_Launch8", "Key_Launch9", "Key_LaunchA", "Key_LaunchB", "Key_LaunchC", "Key_LaunchD", "Key_LaunchE", "Key_LaunchF",
      "Key_LaunchG", "Key_LaunchH", "Key_MonBrightnessUp", "Key_MonBrightnessDown", "Key_KeyboardLightOnOff", "Key_KeyboardBrightnessUp", "Key_KeyboardBrightnessDown", "Key_PowerOff", "Key_WakeUp", "Key_Eject",
      "Key_ScreenSaver", "Key_WWW", "Key_Memo", "Key_LightBulb", "Key_Shop", "Key_History", "Key_AddFavorite", "Key_HotLinks", "Key_BrightnessAdjust", "Key_Finance",
      "Key_Community", "Key_AudioRewind", "Key_BackForward", "Key_ApplicationLeft", "Key_ApplicationRight", "Key_Book", "Key_CD", "Key_Calculator", "Key_ToDoList", "Key_ClearGrab",
      "Key_Close", "Key_Copy", "Key_Cut", "Key_Display", "Key_DOS", "Key_Documents", "Key_Excel", "Key_Explorer", "Key_Game", "Key_Go",
      "Key_iTouch", "Key_LogOff", "Key_Market", "Key_Meeting", "Key_MenuKB", "Key_MenuPB", "Key_MySites", "Key_News", "Key_OfficeHome", "Key_Option",
      "Key_Paste", "Key_Phone", "Key_Calendar", "Key_Reply", "Key_Reload", "Key_RotateWindows", "Key_RotationPB", "Key_RotationKB", "Key_Save", "Key_Send",
      "Key_Spell", "Key_SplitScreen", "Key_Support", "Key_TaskPane", "Key_Terminal", "Key_Tools", "Key_Travel", "Key_Video", "Key_Word", "Key_Xfer",
      "Key_ZoomIn", "Key_ZoomOut", "Key_Away", "Key_Messenger", "Key_WebCam", "Key_MailForward", "Key_Pictures", "Key_Music", "Key_Battery", "Key_Bluetooth",
      "Key_WLAN", "Key_UWB", "Key_AudioForward", "Key_AudioRepeat", "Key_AudioRandomPlay", "Key_Subtitle", "Key_AudioCycleTrack", "Key_Time", "Key_Hibernate", "Key_View",
      "Key_TopMenu", "Key_PowerDown", "Key_Suspend", "Key_ContrastAdjust", "Key_TouchpadToggle", "Key_TouchpadOn", "Key_TouchpadOff", "Key_MicMute", "Key_Red", "Key_Green",
      "Key_Yellow", "Key_Blue", "Key_ChannelUp", "Key_ChannelDown", "Key_Guide", "Key_Info", "Key_Settings", "Key_MicVolumeUp", "Key_MicVolumeDown", "Key_New",
      "Key_Open", "Key_Find", "Key_Undo", "Key_Redo", "Key_MediaLast", "Key_unknown", "Key_Call", "Key_Camera", "Key_CameraFocus", "Key_Context1",
      "Key_Context2", "Key_Context3", "Key_Context4", "Key_Flip", "Key_Hangup", "Key_No", "Key_Select", "Key_Yes", "Key_ToggleCallHangup", "Key_VoiceDial",
      "Key_LastNumberRedial", "Key_Execute", "Key_Printer", "Key_Play", "Key_Sleep", "Key_Zoom", "Key_Exit", "Key_Cancel"
    }),
    std::vector<int>({
      Qt::Key_Escape, Qt::Key_Tab, Qt::Key_Backtab, Qt::Key_Backspace, Qt::Key_Return, Qt::Key_Enter, Qt::Key_Insert, Qt::Key_Delete, Qt::Key_Pause, Qt::Key_Print,
      Qt::Key_SysReq, Qt::Key_Clear, Qt::Key_Home, Qt::Key_End, Qt::Key_Left, Qt::Key_Up, Qt::Key_Right, Qt::Key_Down, Qt::Key_PageUp, Qt::Key_PageDown,
      Qt::Key_Shift, Qt::Key_Control, Qt::Key_Meta, Qt::Key_Alt, Qt::Key_AltGr, Qt::Key_CapsLock, Qt::Key_NumLock, Qt::Key_ScrollLock, Qt::Key_F1, Qt::Key_F2,
      Qt::Key_F3, Qt::Key_F4, Qt::Key_F5, Qt::Key_F6, Qt::Key_F7, Qt::Key_F8, Qt::Key_F9, Qt::Key_F10, Qt::Key_F11, Qt::Key_F12,
      Qt::Key_F13, Qt::Key_F14, Qt::Key_F15, Qt::Key_F16, Qt::Key_F17, Qt::Key_F18, Qt::Key_F19, Qt::Key_F20, Qt::Key_F21, Qt::Key_F22,
      Qt::Key_F23, Qt::Key_F24, Qt::Key_F25, Qt::Key_F26, Qt::Key_F27, Qt::Key_F28, Qt::Key_F29, Qt::Key_F30, Qt::Key_F31, Qt::Key_F32,
      Qt::Key_F33, Qt::Key_F34, Qt::Key_F35, Qt::Key_Super_L, Qt::Key_Super_R, Qt::Key_Menu, Qt::Key_Hyper_L, Qt::Key_Hyper_R, Qt::Key_Help, Qt::Key_Direction_L,
      Qt::Key_Direction_R, Qt::Key_Space, Qt::Key_Exclam, Qt::Key_QuoteDbl, Qt::Key_NumberSign, Qt::Key_Dollar, Qt::Key_Percent, Qt::Key_Ampersand, Qt::Key_Apostrophe, Qt::Key_ParenLeft,
      Qt::Key_ParenRight, Qt::Key_Asterisk, Qt::Key_Plus, Qt::Key_Comma, Qt::Key_Minus, Qt::Key_Period, Qt::Key_Slash, Qt::Key_0, Qt::Key_1, Qt::Key_2,
      Qt::Key_3, Qt::Key_4, Qt::Key_5, Qt::Key_6, Qt::Key_7, Qt::Key_8, Qt::Key_9, Qt::Key_Colon, Qt::Key_Semicolon, Qt::Key_Less,
      Qt::Key_Equal, Qt::Key_Greater, Qt::Key_Question, Qt::Key_At, Qt::Key_A, Qt::Key_B, Qt::Key_C, Qt::Key_D, Qt::Key_E, Qt::Key_F,
      Qt::Key_G, Qt::Key_H, Qt::Key_I, Qt::Key_J, Qt::Key_K, Qt::Key_L, Qt::Key_M, Qt::Key_N, Qt::Key_O, Qt::Key_P,
      Qt::Key_Q, Qt::Key_R, Qt::Key_S, Qt::Key_T, Qt::Key_U, Qt::Key_V, Qt::Key_W, Qt::Key_X, Qt::Key_Y, Qt::Key_Z,
      Qt::Key_BracketLeft, Qt::Key_Backslash, Qt::Key_BracketRight, Qt::Key_AsciiCircum, Qt::Key_Underscore, Qt::Key_QuoteLeft, Qt::Key_BraceLeft, Qt::Key_Bar, Qt::Key_BraceRight, Qt::Key_AsciiTilde,
      Qt::Key_nobreakspace, Qt::Key_exclamdown, Qt::Key_cent, Qt::Key_sterling, Qt::Key_currency, Qt::Key_yen, Qt::Key_brokenbar, Qt::Key_section, Qt::Key_diaeresis, Qt::Key_copyright,
      Qt::Key_ordfeminine, Qt::Key_guillemotleft, Qt::Key_notsign, Qt::Key_hyphen, Qt::Key_registered, Qt::Key_macron, Qt::Key_degree, Qt::Key_plusminus, Qt::Key_twosuperior, Qt::Key_threesuperior,
      Qt::Key_acute, Qt::Key_micro, Qt::Key_paragraph, Qt::Key_periodcentered, Qt::Key_cedilla, Qt::Key_onesuperior, Qt::Key_masculine, Qt::Key_guillemotright, Qt::Key_onequarter, Qt::Key_onehalf,
      Qt::Key_threequarters, Qt::Key_questiondown, Qt::Key_Agrave, Qt::Key_Aacute, Qt::Key_Acircumflex, Qt::Key_Atilde, Qt::Key_Adiaeresis, Qt::Key_Aring, Qt::Key_AE, Qt::Key_Ccedilla,
      Qt::Key_Egrave, Qt::Key_Eacute, Qt::Key_Ecircumflex, Qt::Key_Ediaeresis, Qt::Key_Igrave, Qt::Key_Iacute, Qt::Key_Icircumflex, Qt::Key_Idiaeresis, Qt::Key_ETH, Qt::Key_Ntilde,
      Qt::Key_Ograve, Qt::Key_Oacute, Qt::Key_Ocircumflex, Qt::Key_Otilde, Qt::Key_Odiaeresis, Qt::Key_multiply, Qt::Key_Ooblique, Qt::Key_Ugrave, Qt::Key_Uacute, Qt::Key_Ucircumflex,
      Qt::Key_Udiaeresis, Qt::Key_Yacute, Qt::Key_THORN, Qt::Key_ssharp, Qt::Key_division, Qt::Key_ydiaeresis, Qt::Key_Multi_key, Qt::Key_Codeinput, Qt::Key_SingleCandidate, Qt::Key_MultipleCandidate,
      Qt::Key_PreviousCandidate, Qt::Key_Mode_switch, Qt::Key_Kanji, Qt::Key_Muhenkan, Qt::Key_Henkan, Qt::Key_Romaji, Qt::Key_Hiragana, Qt::Key_Katakana, Qt::Key_Hiragana_Katakana, Qt::Key_Zenkaku,
      Qt::Key_Hankaku, Qt::Key_Zenkaku_Hankaku, Qt::Key_Touroku, Qt::Key_Massyo, Qt::Key_Kana_Lock, Qt::Key_Kana_Shift, Qt::Key_Eisu_Shift, Qt::Key_Eisu_toggle, Qt::Key_Hangul, Qt::Key_Hangul_Start,
      Qt::Key_Hangul_End, Qt::Key_Hangul_Hanja, Qt::Key_Hangul_Jamo, Qt::Key_Hangul_Romaja, Qt::Key_Hangul_Jeonja, Qt::Key_Hangul_Banja, Qt::Key_Hangul_PreHanja, Qt::Key_Hangul_PostHanja, Qt::Key_Hangul_Special, Qt::Key_Dead_Grave,
      Qt::Key_Dead_Acute, Qt::Key_Dead_Circumflex, Qt::Key_Dead_Tilde, Qt::Key_Dead_Macron, Qt::Key_Dead_Breve, Qt::Key_Dead_Abovedot, Qt::Key_Dead_Diaeresis, Qt::Key_Dead_Abovering, Qt::Key_Dead_Doubleacute, Qt::Key_Dead_Caron,
      Qt::Key_Dead_Cedilla, Qt::Key_Dead_Ogonek, Qt::Key_Dead_Iota, Qt::Key_Dead_Voiced_Sound, Qt::Key_Dead_Semivoiced_Sound, Qt::Key_Dead_Belowdot, Qt::Key_Dead_Hook, Qt::Key_Dead_Horn, Qt::Key_Dead_Stroke, Qt::Key_Dead_Abovecomma,
      Qt::Key_Dead_Abovereversedcomma, Qt::Key_Dead_Doublegrave, Qt::Key_Dead_Belowring, Qt::Key_Dead_Belowmacron, Qt::Key_Dead_Belowcircumflex, Qt::Key_Dead_Belowtilde, Qt::Key_Dead_Belowbreve, Qt::Key_Dead_Belowdiaeresis, Qt::Key_Dead_Invertedbreve, Qt::Key_Dead_Belowcomma,
      Qt::Key_Dead_Currency, Qt::Key_Dead_a, Qt::Key_Dead_A, Qt::Key_Dead_e, Qt::Key_Dead_E, Qt::Key_Dead_i, Qt::Key_Dead_I, Qt::Key_Dead_o, Qt::Key_Dead_O, Qt::Key_Dead_u,
      Qt::Key_Dead_U, Qt::Key_Dead_Small_Schwa, Qt::Key_Dead_Capital_Schwa, Qt::Key_Dead_Greek, Qt::Key_Dead_Lowline, Qt::Key_Dead_Aboveverticalline, Qt::Key_Dead_Belowverticalline, Qt::Key_Dead_Longsolidusoverlay, Qt::Key_Back, Qt::Key_Forward,
      Qt::Key_Stop, Qt::Key_Refresh, Qt::Key_VolumeDown, Qt::Key_VolumeMute, Qt::Key_VolumeUp, Qt::Key_BassBoost, Qt::Key_BassUp, Qt::Key_BassDown, Qt::Key_TrebleUp, Qt::Key_TrebleDown,
      Qt::Key_MediaPlay, Qt::Key_MediaStop, Qt::Key_MediaPrevious, Qt::Key_MediaNext, Qt::Key_MediaRecord, Qt::Key_MediaPause, Qt::Key_MediaTogglePlayPause, Qt::Key_HomePage, Qt::Key_Favorites, Qt::Key_Search,
      Qt::Key_Standby, Qt::Key_OpenUrl, Qt::Key_LaunchMail, Qt::Key_LaunchMedia, Qt::Key_Launch0, Qt::Key_Launch1, Qt::Key_Launch2, Qt::Key_Launch3, Qt::Key_Launch4, Qt::Key_Launch5,
      Qt::Key_Launch6, Qt::Key_Launch7, Qt::Key_Launch8, Qt::Key_Launch9, Qt::Key_LaunchA, Qt::Key_LaunchB, Qt::Key_LaunchC, Qt::Key_LaunchD, Qt::Key_LaunchE, Qt::Key_LaunchF,
      Qt::Key_LaunchG, Qt::Key_LaunchH, Qt::Key_MonBrightnessUp, Qt::Key_MonBrightnessDown, Qt::Key_KeyboardLightOnOff, Qt::Key_KeyboardBrightnessUp, Qt::Key_KeyboardBrightnessDown, Qt::Key_PowerOff, Qt::Key_WakeUp, Qt::Key_Eject,
      Qt::Key_ScreenSaver, Qt::Key_WWW, Qt::Key_Memo, Qt::Key_LightBulb, Qt::Key_Shop, Qt::Key_History, Qt::Key_AddFavorite, Qt::Key_HotLinks, Qt::Key_BrightnessAdjust, Qt::Key_Finance,
      Qt::Key_Community, Qt::Key_AudioRewind, Qt::Key_BackForward, Qt::Key_ApplicationLeft, Qt::Key_ApplicationRight, Qt::Key_Book, Qt::Key_CD, Qt::Key_Calculator, Qt::Key_ToDoList, Qt::Key_ClearGrab,
      Qt::Key_Close, Qt::Key_Copy, Qt::Key_Cut, Qt::Key_Display, Qt::Key_DOS, Qt::Key_Documents, Qt::Key_Excel, Qt::Key_Explorer, Qt::Key_Game, Qt::Key_Go,
      Qt::Key_iTouch, Qt::Key_LogOff, Qt::Key_Market, Qt::Key_Meeting, Qt::Key_MenuKB, Qt::Key_MenuPB, Qt::Key_MySites, Qt::Key_News, Qt::Key_OfficeHome, Qt::Key_Option,
      Qt::Key_Paste, Qt::Key_Phone, Qt::Key_Calendar, Qt::Key_Reply, Qt::Key_Reload, Qt::Key_RotateWindows, Qt::Key_RotationPB, Qt::Key_RotationKB, Qt::Key_Save, Qt::Key_Send,
      Qt::Key_Spell, Qt::Key_SplitScreen, Qt::Key_Support, Qt::Key_TaskPane, Qt::Key_Terminal, Qt::Key_Tools, Qt::Key_Travel, Qt::Key_Video, Qt::Key_Word, Qt::Key_Xfer,
      Qt::Key_ZoomIn, Qt::Key_ZoomOut, Qt::Key_Away, Qt::Key_Messenger, Qt::Key_WebCam, Qt::Key_MailForward, Qt::Key_Pictures, Qt::Key_Music, Qt::Key_Battery, Qt::Key_Bluetooth,
      Qt::Key_WLAN, Qt::Key_UWB, Qt::Key_AudioForward, Qt::Key_AudioRepeat, Qt::Key_AudioRandomPlay, Qt::Key_Subtitle, Qt::Key_AudioCycleTrack, Qt::Key_Time, Qt::Key_Hibernate, Qt::Key_View,
      Qt::Key_TopMenu, Qt::Key_PowerDown, Qt::Key_Suspend, Qt::Key_ContrastAdjust, Qt::Key_TouchpadToggle, Qt::Key_TouchpadOn, Qt::Key_TouchpadOff, Qt::Key_MicMute, Qt::Key_Red, Qt::Key_Green,
      Qt::Key_Yellow, Qt::Key_Blue, Qt::Key_ChannelUp, Qt::Key_ChannelDown, Qt::Key_Guide, Qt::Key_Info, Qt::Key_Settings, Qt::Key_MicVolumeUp, Qt::Key_MicVolumeDown, Qt::Key_New,
      Qt::Key_Open, Qt::Key_Find, Qt::Key_Undo, Qt::Key_Redo, Qt::Key_MediaLast, Qt::Key_unknown, Qt::Key_Call, Qt::Key_Camera, Qt::Key_CameraFocus, Qt::Key_Context1,
      Qt::Key_Context2, Qt::Key_Context3, Qt::Key_Context4, Qt::Key_Flip, Qt::Key_Hangup, Qt::Key_No, Qt::Key_Select, Qt::Key_Yes, Qt::Key_ToggleCallHangup, Qt::Key_VoiceDial,
      Qt::Key_LastNumberRedial, Qt::Key_Execute, Qt::Key_Printer, Qt::Key_Play, Qt::Key_Sleep, Qt::Key_Zoom, Qt::Key_Exit, Qt::Key_Cancel
    })
  );

  qml_module.add_type<QObject>("QObject")
    .method("deleteLater", &QObject::deleteLater);
  qml_module.method("connect_destroyed_signal", [] (QObject& obj, jl_function_t* jl_f)
  {
    QObject::connect(&obj, &QObject::destroyed, [jl_f](QObject* o)
    {
      static JuliaFunction f(jl_f);
      f(o);
    });
  });

  qml_module.add_type<QSize>("QSize")
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
    .method("toString", [] (const QUrl& url) { return url.toString(); });
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
    .method("connect_value_changed", [] (QQmlPropertyMap& propmap, jl_value_t* julia_property_map, jl_function_t* callback)
    {
      auto conn = QObject::connect(&propmap, &QQmlPropertyMap::valueChanged, [=](const QString& key, const QVariant& newvalue)
      {
        const jlcxx::JuliaFunction on_value_changed(callback);
        jl_value_t* julia_propmap = julia_property_map;
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

  auto qqmlcontext_wrapper = qml_module.add_type<QQmlContext>("QQmlContext", julia_base_type<QObject>())
    .constructor<QQmlContext*>()
    .constructor<QQmlContext*, QObject*>()
    .method("context_property", &QQmlContext::contextProperty)
    .method("set_context_object", &QQmlContext::setContextObject)
    .method("_set_context_property", static_cast<void(QQmlContext::*)(const QString&, const QVariant&)>(&QQmlContext::setContextProperty))
    .method("_set_context_property", static_cast<void(QQmlContext::*)(const QString&, QObject*)>(&QQmlContext::setContextProperty))
    .method("context_object", &QQmlContext::contextObject);

  qml_module.add_type<QQmlEngine>("QQmlEngine", julia_base_type<QObject>())
    .method("clearComponentCache", &QQmlEngine::clearComponentCache)
    .method("clearSingletons", &QQmlEngine::clearSingletons)
    .method("root_context", &QQmlEngine::rootContext)
    .method("quit", &QQmlEngine::quit);

  qqmlcontext_wrapper.method("engine", &QQmlContext::engine);

  qml_module.add_type<QQmlApplicationEngine>("QQmlApplicationEngine", julia_base_type<QQmlEngine>())
    .constructor<QString>() // Construct with path to QML
    .method("rootObjects", &QQmlApplicationEngine::rootObjects)
    .method("load_into_engine", [] (QQmlApplicationEngine* e, const QString& qmlpath)
    {
      bool success = false;
      auto conn = QObject::connect(e, &QQmlApplicationEngine::objectCreated, [&] (QObject* obj, const QUrl& url) { success = (obj != nullptr); });
      e->load(qmlpath);
      QObject::disconnect(conn);
      if(!success)
      {
        e->exit(1);
      }
      return success;
    });

  qml_module.method("qt_prefix_path", []() { return QLibraryInfo::path(QLibraryInfo::PrefixPath); });

  auto qquickitem_type = qml_module.add_type<QQuickItem>("QQuickItem", julia_base_type<QObject>());

  qml_module.add_type<QWindow>("QWindow", julia_base_type<QObject>())
    .method("destroy", &QWindow::destroy);
  qml_module.add_type<QQuickWindow>("QQuickWindow", julia_base_type<QWindow>())
    .method("content_item", &QQuickWindow::contentItem);

  qquickitem_type.method("window", &QQuickItem::window);

  qml_module.method("effectiveDevicePixelRatio", [] (QQuickWindow& w)
  {
    return w.effectiveDevicePixelRatio();
  });

  qml_module.add_type<QQuickView>("QQuickView", julia_base_type<QQuickWindow>())
    .method("set_source", &QQuickView::setSource)
    .method("show", &QQuickView::show) // not exported: conflicts with Base.show
    .method("engine", &QQuickView::engine)
    .method("root_object", &QQuickView::rootObject);

  qml_module.add_type<qmlwrap::JuliaPaintedItem>("JuliaPaintedItem", julia_base_type<QQuickItem>());

  qml_module.add_type<QQmlComponent>("QQmlComponent", julia_base_type<QObject>())
    .method("set_data", &QQmlComponent::setData);
  // We manually add this constructor, since no finalizer will be added here. The component is destroyed together with the engine.
  qml_module.method("QQmlComponent", [] (QQmlEngine* e) { return new QQmlComponent(e,e); });
  qml_module.method("create", [](QQmlComponent* comp, QQmlContext* context)
  {
    if(!comp->isReady())
    {
      qWarning() << "QQmlComponent is not ready, aborting create. Errors were: " << comp->errors();
      return;
    }

    QObject* obj = comp->create(context);
    if(context != nullptr)
    {
      obj->setParent(context); // setting this makes sure the new object gets deleted
    }
  });

#if (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
  qml_module.method("qputenv", [] (const char* varName, QByteArray value) { qputenv(varName, value); });
#else
  qml_module.method("qputenv", qputenv);
#endif
  qml_module.method("qgetenv", qgetenv);
  qml_module.method("qunsetenv", qunsetenv);

  // App manager functions
  qml_module.add_type<qmlwrap::ApplicationManager>("ApplicationManager");
  qml_module.method("init_qmlapplicationengine", []() { return qmlwrap::ApplicationManager::instance().init_qmlapplicationengine(); });
  qml_module.method("init_qmlengine", []() { return qmlwrap::ApplicationManager::instance().init_qmlengine(); });
  qml_module.method("get_qmlengine", []() { return qmlwrap::ApplicationManager::instance().get_qmlengine(); });
  qml_module.method("init_qquickview", []() { return qmlwrap::ApplicationManager::instance().init_qquickview(); });
  qml_module.method("cleanup", []() { qmlwrap::ApplicationManager::instance().cleanup(); });
  qml_module.method("qmlcontext", []() { return qmlwrap::ApplicationManager::instance().root_context(); });
  qml_module.method("exec", []() { qmlwrap::ApplicationManager::instance().exec(); });
  qml_module.method("process_events", qmlwrap::ApplicationManager::process_events);
  qml_module.method("add_import_path", [](std::string path) { qmlwrap::ApplicationManager::instance().add_import_path(path); });

  qml_module.add_type<QTimer>("QTimer", julia_base_type<QObject>())
    .method("start", [] (QTimer& t) { t.start(); } )
    .method("stop", &QTimer::stop);

  // Emit signals helper
  qml_module.method("emit", [](const char *signal_name, const QVariantList& args) {
    using namespace qmlwrap;
    JuliaSignals *julia_signals = ApplicationManager::instance().julia_api()->juliaSignals();
    if (julia_signals == nullptr)
    {
      throw std::runtime_error("No signals available");
    }
    julia_signals->emit_signal(signal_name, args);
  });

  // Function to register a function
  qml_module.method("qmlfunction", [](const QString &name, jl_function_t *f) {
    qmlwrap::ApplicationManager::instance().julia_api()->register_function(name, f);
  });

  qml_module.add_type<QPaintDevice>("QPaintDevice")
    .method("width", &QPaintDevice::width)
    .method("height", &QPaintDevice::height)
    .method("logicalDpiX", &QPaintDevice::logicalDpiX)
    .method("logicalDpiY", &QPaintDevice::logicalDpiY);
  qml_module.add_type<QPainter>("QPainter")
    .method("device", &QPainter::device);

  qml_module.add_type<QAbstractItemModel>("QAbstractItemModel", julia_base_type<QObject>());
  qml_module.add_type<QAbstractTableModel>("QAbstractTableModel", julia_base_type<QAbstractItemModel>());
  qml_module.add_type<qmlwrap::JuliaItemModel>("JuliaItemModel", julia_base_type<QAbstractTableModel>())
    .method("emit_data_changed", &qmlwrap::JuliaItemModel::emit_data_changed)
    .method("emit_header_data_changed", &qmlwrap::JuliaItemModel::emit_header_data_changed)
    .method("begin_reset_model", &qmlwrap::JuliaItemModel::begin_reset_model)
    .method("end_reset_model", &qmlwrap::JuliaItemModel::end_reset_model)
    .method("begin_insert_rows", &qmlwrap::JuliaItemModel::begin_insert_rows)
    .method("end_insert_rows", &qmlwrap::JuliaItemModel::end_insert_rows)
    .method("begin_move_rows", &qmlwrap::JuliaItemModel::begin_move_rows)
    .method("end_move_rows", &qmlwrap::JuliaItemModel::end_move_rows)
    .method("begin_remove_rows", &qmlwrap::JuliaItemModel::begin_remove_rows)
    .method("end_remove_rows", &qmlwrap::JuliaItemModel::end_remove_rows)
    .method("begin_insert_columns", &qmlwrap::JuliaItemModel::begin_insert_columns)
    .method("end_insert_columns", &qmlwrap::JuliaItemModel::end_insert_columns)
    .method("begin_move_columns", &qmlwrap::JuliaItemModel::begin_move_columns)
    .method("end_move_columns", &qmlwrap::JuliaItemModel::end_move_columns)
    .method("begin_remove_columns", &qmlwrap::JuliaItemModel::begin_remove_columns)
    .method("end_remove_columns", &qmlwrap::JuliaItemModel::end_remove_columns)
    .method("default_role_names", &qmlwrap::JuliaItemModel::default_role_names)
    .method("get_julia_data", &qmlwrap::JuliaItemModel::get_julia_data);

  qml_module.method("new_item_model", [] (jl_value_t* modeldata) { return jlcxx::create<qmlwrap::JuliaItemModel>(modeldata); });

  qml_module.set_override_module(jl_base_module);
  qml_module.method("getindex", [](const QVariantMap& m, const QString& key) { return m[key]; });
  qml_module.unset_override_module();

  qml_module.add_type<QOpenGLFramebufferObjectFormat>("QOpenGLFramebufferObjectFormat")
    .method("internalTextureFormat", &QOpenGLFramebufferObjectFormat::internalTextureFormat)
    .method("textureTarget", &QOpenGLFramebufferObjectFormat::textureTarget);

  qml_module.add_type<QOpenGLFramebufferObject>("QOpenGLFramebufferObject")
    .method("size", &QOpenGLFramebufferObject::size)
    .method("handle", &QOpenGLFramebufferObject::handle)
    .method("isValid", &QOpenGLFramebufferObject::isValid)
    .method("bind", &QOpenGLFramebufferObject::bind)
    .method("release", &QOpenGLFramebufferObject::release)
    .method("format", &QOpenGLFramebufferObject::format)
    .method("texture", &QOpenGLFramebufferObject::texture)
    .method("addColorAttachment", static_cast<void(QOpenGLFramebufferObject::*)(int,int,GLenum)>(&QOpenGLFramebufferObject::addColorAttachment))
    .method("textures", [] (const QOpenGLFramebufferObject& fbo)
    {
      auto v = fbo.textures();
      return std::vector<GLuint>(v.begin(), v.end());
    });

  qml_module.method("graphicsApi", QQuickWindow::graphicsApi);
  qml_module.method("setGraphicsApi", QQuickWindow::setGraphicsApi);

  qml_module.method("__test_add_double!", [] (double& result, QVariant var)
  {
    result += var.value<double>();
  });

  qml_module.method("__test_add_double_ref!", [] (double& result, const QVariant& var)
  {
    result += var.value<double>();
  });

  qml_module.method("__test_return_qvariant", [] (double val)
  {
    static QVariant var;
    var.setValue(val);
    return var;
  });

  qml_module.method("__test_return_qvariant_ref", [] (double val) -> const QVariant&
  {
    static QVariant var;
    var.setValue(val);
    return var;
  });

  qml_module.add_type<QFileSystemWatcher>("QFileSystemWatcher")
    .constructor<QObject*>(jlcxx::finalize_policy::no)
    .method("addPath", &QFileSystemWatcher::addPath);
  qml_module.method("connect_file_changed_signal", [] (QFileSystemWatcher& watcher, jl_function_t* jl_f)
  {
    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged, [jl_f](const QString& path)
    {
      static JuliaFunction f(jl_f);
      f(path);
    });
  });
}
