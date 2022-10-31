#ifndef QML_JULIAITEMMODEL_H
#define QML_JULIAITEMMODEL_H

#include <map>
#include <string>

#include <QAbstractTableModel>

#include "jlcxx/functions.hpp"

namespace qmlwrap
{

/// Wrap Julia composite types
class JuliaItemModel : public QAbstractTableModel
{
  Q_OBJECT
public:
  static jl_module_t* m_qml_mod;

  JuliaItemModel(jl_value_t* data, QObject* parent = nullptr);
  ~JuliaItemModel();

  int	rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int	columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
  bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  QHash<int,QByteArray> roleNames() const override;

  // Same interface as in Qt6 labs QQmlTableModel
  Q_INVOKABLE void clear();
  Q_INVOKABLE void appendRow(const QVariant& row);
  Q_INVOKABLE void insertRow(int rowIndex, const QVariant& row);
  Q_INVOKABLE void moveRow(int fromRowIndex, int toRowIndex, int rows = 1);
  Q_INVOKABLE void removeRow(int rowIndex, int rows = 1);
  Q_INVOKABLE void setRow(int rowIndex, const QVariant& row);

  Q_INVOKABLE void appendColumn(const QVariant& column);
  Q_INVOKABLE void insertColumn(int columnIndex, const QVariant& column);
  Q_INVOKABLE void moveColumn(int fromColumnIndex, int toColumnIndex, int columns = 1);
  Q_INVOKABLE void removeColumn(int columnIndex, int columns = 1);
  Q_INVOKABLE void setColumn(int columnIndex, const QVariant& column);
  
  // Called from Julia
  void emit_data_changed(int startrow, int startcol, int endrow, int endcol);
  void emit_header_data_changed(Qt::Orientation orientation, int first, int last);
  void begin_reset_model();
  void end_reset_model();
  void begin_insert_rows(int first, int last);
  void end_insert_rows();
  bool begin_move_rows(int fromIndex, int toIndex, int count);
  void end_move_rows();
  void begin_remove_rows(int fromIndex, int count);
  void end_remove_rows();
  void begin_insert_columns(int first, int last);
  void end_insert_columns();
  bool begin_move_columns(int fromIndex, int toIndex, int count);
  void end_move_columns();
  void begin_remove_columns(int fromIndex, int count);
  void end_remove_columns();
  QHash<int,QByteArray> default_role_names() const;
  jl_value_t* get_julia_data() const;

private:
  jl_value_t* m_data;
};

}

#endif
