#ifndef QML_LISTMODEL_H
#define QML_LISTMODEL_H

#include <string>
#include <map>

#include <QAbstractListModel>
#include <QJSValue>
#include <QObject>

#include "type_conversion.hpp"

namespace qmlwrap
{

/// Wrap Julia composite types
class ListModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
  /// Construction using an Array{Any,1}. f should be supplied as an update function to update the source array in case it is not an array of boxed values.
  ListModel(const cxx_wrap::ArrayRef<jl_value_t*>& array, jl_function_t* f = nullptr, QObject* parent = 0);
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

  // Additional QML interface
  /// This overloads append and insert to take a list of variants instead of a dictionary
  Q_INVOKABLE void append(const QVariantList& argvariants);
  Q_INVOKABLE void insert(int index, const QVariantList& argvariants);

  // Called from Julia
  void addrole(const std::string& name, jl_function_t* getter, jl_function_t* setter = nullptr);
  void setconstructor(jl_function_t* constructor);

Q_SIGNALS:
  void countChanged();

private:
  // Update the original array in case we are working with a boxed copy
  void do_update(int index, int count, const QVector<int> &roles);
  void do_update();

  jl_function_t* rolesetter(int role) const;
  jl_function_t* rolegetter(int role) const;
  cxx_wrap::ArrayRef<jl_value_t*> m_array;
  QHash<int, QByteArray> m_rolenames;
  jl_function_t* m_constructor = nullptr;
  jl_function_t* m_update_array = nullptr; // Function used to update an array with non-boxed contents
  bool m_custom_roles = false;
  std::vector<jl_function_t*> m_getters;
  std::vector<jl_function_t*> m_setters;
};

}

#endif
