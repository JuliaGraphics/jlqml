#ifndef QML_MODELDATA_H
#define QML_MODELDATA_H

#include <map>
#include <string>

#include <QAbstractListModel>

#include "jlcxx/functions.hpp"

namespace qmlwrap
{

/// Wrap Julia composite types
class ModelData : public QObject
{
  Q_OBJECT
public:
  static jl_module_t* m_qml_mod;

  ModelData(jl_value_t* data, QAbstractItemModel* parent);
  ~ModelData();


  int rowcount() const;
  int colcount() const;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  QStringList roles() const;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  Qt::ItemFlags flags(const QModelIndex &index) const;
  void append_list(const QVariantList& argvariants);
  void remove(int index);

  struct Move
  {
    int from;
    int to;
    int count;
  };

  Move normalizemove(int from, int to, int count);
  void move(const Move& m);
  void clear();
  
  // Called from Julia
  void emit_roles_changed();
  void emit_data_changed(int index, int count, const std::vector<int>& roles);
  void push_back(jl_value_t* val);
  jl_value_t* get_julia_data() const;

Q_SIGNALS:
  void countChanged();
  void rolesChanged();
  void dataChanged(int index, int count, const std::vector<int>& roles);

private:
  jl_value_t* m_data;
};

}

#endif
