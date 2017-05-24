#ifndef QML_LISTMODEL_H
#define QML_LISTMODEL_H

#include <map>
#include <string>

#include <QAbstractListModel>
#include <QJSValue>
#include <QObject>

#include "type_conversion.hpp"

namespace qmlwrap
{

/// Encapsulate a list of functions, managing their lifetime using (un)protect_from_gc
class FunctionList
{
public:
  jl_function_t* get(const size_t i) const;
  void set(const size_t i, jl_function_t* val);
  void clear();
  void push_back(jl_function_t* f);
  void erase(const size_t idx);
  size_t size() const;
  ~FunctionList();
private:
  void protect(jl_function_t* f);
  void unprotect(jl_function_t* f);
  std::vector<jl_function_t*> m_functions;
};

/// Wrap Julia composite types
class ListModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)
  Q_PROPERTY(QStringList roles READ roles NOTIFY rolesChanged)
public:
  /// Construction using an Array{Any,1}. f should be supplied as an update function to update the source array in case it is not an array of boxed values.
  ListModel(const jlcxx::ArrayRef<jl_value_t*>& array, jl_function_t* f = nullptr, QObject* parent = 0);
  virtual ~ListModel();

  // QAbstractItemModel interface
  virtual int	rowCount(const QModelIndex& parent = QModelIndex()) const;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  // TODO: Add headerData
  virtual QHash<int, QByteArray> roleNames() const;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  Qt::ItemFlags flags(const QModelIndex &index) const;

  // Mimic QML Listmodel interface
  Q_INVOKABLE void append(const QJSValue& record);
  Q_INVOKABLE void insert(int index, const QJSValue& record);
  Q_INVOKABLE void setProperty(int index, const QString& property, const QVariant& value);
  Q_INVOKABLE void remove(int index);
  Q_INVOKABLE void move(int from, int to, int count);
  Q_INVOKABLE void clear();
  int count() const;

  // Called from Julia
  void addrole(const std::string& name, jl_function_t* getter, jl_function_t* setter = nullptr);
  void setrole(const int idx, const std::string& name, jl_function_t* getter, jl_function_t* setter = nullptr);
  void removerole(const int idx);
  void removerole(const std::string& name);
  void setconstructor(jl_function_t* constructor);

  // Roles property
  QStringList roles() const;

Q_SIGNALS:
  void countChanged();
  void rolesChanged();

private:
  // Update the original array in case we are working with a boxed copy
  void do_update(int index, int count, const QVector<int> &roles);
  void do_update();

  /// This overloads append and insert to take a list of variants instead of a dictionary
  void append_list(const QVariantList& argvariants);
  void insert_list(int index, const QVariantList& argvariants);

  jlcxx::JuliaFunction rolesetter(int role) const;
  jlcxx::JuliaFunction rolegetter(int role) const;
  jlcxx::ArrayRef<jl_value_t*> m_array;
  QHash<int, QByteArray> m_rolenames;
  jl_function_t* m_constructor = nullptr;
  jl_function_t* m_update_array = nullptr; // Function used to update an array with non-boxed contents
  bool m_custom_roles = false;
  FunctionList m_getters;
  FunctionList m_setters;
};

}

#endif
