#ifndef QML_LISTMODEL_H
#define QML_LISTMODEL_H

#include <map>
#include <string>

#include <QAbstractListModel>
#include <QJSValue>
#include <QObject>

#include "modeldata.hpp"

namespace qmlwrap
{

/// Wrap Julia composite types
class ListModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)
  Q_PROPERTY(QStringList roles READ roles NOTIFY rolesChanged)
public:

  ListModel(jl_value_t* data, QObject* parent = 0);
  virtual ~ListModel();

  // QAbstractItemModel interface
  virtual int	rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  // TODO: Add headerData
  virtual QHash<int, QByteArray> roleNames() const override;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  // Mimic QML Listmodel interface
  Q_INVOKABLE void append(const QJSValue& record);
  Q_INVOKABLE void insert(int index, const QJSValue& record);
  Q_INVOKABLE void setProperty(int index, const QString& property, const QVariant& value);
  Q_INVOKABLE void remove(int index);
  Q_INVOKABLE void move(int from, int to, int count);
  Q_INVOKABLE void clear();
  int count() const;

  // Called from Julia
  ModelData* get_model_data() const;

  // Roles property
  QStringList roles() const;
Q_SIGNALS:
  void countChanged();
  void rolesChanged();

private:

  /// This overloads append and insert to take a list of variants instead of a dictionary
  void append_list(const QVariantList& argvariants);
  void insert_list(int index, const QVariantList& argvariants);
  void onDataChanged(int index, int count, const std::vector<int>& roles);

  ModelData* m_data;
};

}

#endif
