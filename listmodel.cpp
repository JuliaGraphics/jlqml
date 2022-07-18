#include "jlcxx/functions.hpp"

#include <QDebug>
#include <QQmlListProperty>
#include "listmodel.hpp"

namespace qmlwrap
{

ListModel::ListModel(jl_value_t* data, QObject* parent) : QAbstractListModel(parent)
{
  m_data = new ModelData(data, this);
  QObject::connect(m_data, &ModelData::countChanged, this, &ListModel::countChanged);
  QObject::connect(m_data, &ModelData::rolesChanged, this, &ListModel::rolesChanged);
  QObject::connect(m_data, &ModelData::dataChanged, this, &ListModel::onDataChanged);
}

ListModel::~ListModel()
{
}

int	ListModel::rowCount(const QModelIndex&) const
{
  return m_data->rowcount();
}

QVariant ListModel::data(const QModelIndex& index, int role) const
{
  return m_data->data(index, role);
}

QHash<int, QByteArray> ListModel::roleNames() const
{
  QHash<int, QByteArray> result;
  QStringList rolenames = m_data->roles();
  for(int i = 0; i != rolenames.size(); ++i)
  {
    result[i] = rolenames[i].toUtf8();
  }

  return result;
}

bool ListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  return m_data->setData(index, value, role);
}

Qt::ItemFlags ListModel::flags(const QModelIndex&) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

void ListModel::append_list(const QVariantList& argvariants)
{
  beginInsertRows(QModelIndex(), count(), count());
  m_data->append_list(argvariants);
  endInsertRows();
  emit countChanged();
}

void ListModel::append(const QJSValue& record)
{
  if(record.isArray())
  {
    m_data->append_list(record.toVariant().toList());
    return;
  }

  QVariantList argvariants;
  QStringList rolenames = roles();
  const int nb_roles = rolenames.size();
  for(int i =0; i != nb_roles; ++i)
  {
    auto rolename = rolenames[i];
    if(record.hasProperty(rolename))
    {
      argvariants.push_back(record.property(QString(rolename)).toVariant());
    }
  }
  m_data->append_list(argvariants);
}

Q_INVOKABLE void ListModel::insert(int index, const QJSValue& record)
{
  append(record);
  move(count()-1, index, 1);
}

Q_INVOKABLE void ListModel::insert_list(int index, const QVariantList& argvariants)
{
  append_list(argvariants);
  move(count()-1, index, 1);
}

void ListModel::setProperty(int index, const QString& property, const QVariant& value)
{
  setData(createIndex(index,0), value, roles().indexOf(property));
}

void ListModel::remove(int index)
{
  beginRemoveRows(QModelIndex(), index, index);
  m_data->remove(index);
  endRemoveRows();
  emit countChanged();
}

void ListModel::move(int from, int to, int count)
{
  const ModelData::Move m = m_data->normalizemove(from,to,count);
  if(m.count < 0)
  {
    qWarning() << "Invalid ListModel move from " << from << " to " << to << " with count " << count;
  }

  beginMoveRows(QModelIndex(), m.from, m.from + m.count - 1, QModelIndex(), m.to+m.count);
  m_data->move(m);
  endMoveRows();
}

void ListModel::clear()
{
  beginRemoveRows(QModelIndex(), 0, count());
  m_data->clear();
  endRemoveRows();
}

int ListModel::count() const
{
  return m_data->rowcount();
}

void ListModel::onDataChanged(int index, int count, const std::vector<int>& roles)
{
  emit dataChanged(createIndex(index, 0), createIndex(index + count - 1, 0), QList<int>(roles.begin(), roles.end()));
}

QStringList ListModel::roles() const
{
  return m_data->roles();
}

ModelData* ListModel::get_model_data() const
{
  return m_data;
}

} // namespace qmlwrap
