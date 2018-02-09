#ifndef DATABACKEND_H
#define DATABACKEND_H

#include <QObject>
#include <QSqlDatabase>
#include <QStringList>
#include <memory>
namespace PortableDBBackend
{
class ITableDefinition
{
public:
  virtual QStringList getCreateStatements(int TargetVersion) const=0;
  virtual bool NeedUpdate(int OldVersion, int UpdatedVersion)const =0;
  virtual QStringList getUpdateStatement(int OldVersion, int TargetVersion)const=0;
  virtual QStringList getDeleteStatements() const = 0;

  // optional: fill freshly created tables with values
  virtual QStringList insertInitialRows(int /*TargetVersion*/) const
  { QStringList Empty; return Empty; }
};

// forward declarations
class DataBackend_pImpl;
class DataBackend : public QObject
{
  Q_OBJECT
public:
  DataBackend();
  virtual ~DataBackend();

  // AddTable must be called before InitializeDB
  void AddTable(std::unique_ptr<ITableDefinition> Table); // as we take ownership of the Table the returned unique_ptr will be empty

public slots:
  // this will attempt to open/create/update the db
  // the function will block and return only when the Db is initialized
  bool InitializeDB(const QString& ProposedFilename, QSqlDatabase& DbToInitialize);
  bool DeleteAllData(QSqlDatabase& DbToDelete);

  void setDbVersion(int CurrentVersion); // call in c'tor of derived classes to support DB updates
protected:
	template <class TableType> void addTableType()
  {
    AddTable(std::unique_ptr<ITableDefinition>(new TableType));
  }
	
private:
  // we use the pImpl idiom to separate our internal implementation
  std::unique_ptr<DataBackend_pImpl> m_spPImpl;
};
} // end of namespace
#endif // DATABACKEND_H
