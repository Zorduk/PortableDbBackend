#ifndef DATABACKEND_PIMPL_H
#define DATABACKEND_PIMPL_H

#include <QObject>
#include "databackend.h"
namespace PortableDBBackend
{

class DataBackend_pImpl : public QObject
{
  Q_OBJECT

public:
  DataBackend_pImpl();
  ~DataBackend_pImpl();

signals:
  void DbReady();
  void DbError(); // unrecoverable DB error

public:
  bool InitializeDB(const QString& ProposedFilename, QSqlDatabase& DBToInitialize);
  void setDbVersion(int CurrentVersion);
  void AddTable(std::unique_ptr<ITableDefinition> Table); // as we take ownership of the Table the returned unique_ptr will be empty
	bool DeleteAllData(QSqlDatabase& DbToDelete);

private:
  bool CreateTables(QSqlDatabase& DB);
  bool CheckDatabaseForUpdates(QSqlDatabase& DB);
  bool RunUpdates(QSqlDatabase& DB, int OldVersion);
  bool PrepareDatabaseForUse(QSqlDatabase &DB);
  int GetFileDbVersion(QSqlDatabase& DB); // queries the database to read the version

  // private member
  QString m_Filename;
  std::vector<std::shared_ptr<ITableDefinition> > m_Tables;
  int m_DBVersion; // this is the current version implemented in our  C++ code
};

class DbTableVersion : public ITableDefinition
{
public:
  virtual QStringList getCreateStatements(int TargetVersion) const override;
  virtual bool NeedUpdate(int OldVersion, int UpdatedVersion)const override;
  virtual QStringList getUpdateStatement(int OldVersion, int TargetVersion)const override;
	virtual QStringList getDeleteStatements() const override;

  // optional: fill freshly created tables with values
  virtual QStringList insertInitialRows(int TargetVersion) const override;
};

}
#endif // DATABACKEND_PIMPL_H
