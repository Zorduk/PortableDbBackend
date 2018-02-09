#include "databackend_pimpl.h"

#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QSqlQuery>

namespace PortableDBBackend
{
DataBackend_pImpl::DataBackend_pImpl()
  : QObject(nullptr)
{
  AddTable(std::unique_ptr<ITableDefinition>(new DbTableVersion));
}

DataBackend_pImpl::~DataBackend_pImpl()
{

}

void DataBackend_pImpl::setDbVersion(int CurrentVersion)
{
  m_DBVersion = CurrentVersion;
}

void DataBackend_pImpl::AddTable(std::unique_ptr<ITableDefinition> spTable)
{
  m_Tables.push_back(std::move(spTable));
}

bool DataBackend_pImpl::InitializeDB(const QString& ProposedFilename, QSqlDatabase& DataBase)
{
  // this will attempt to open, if that fails the db is going to be created and initialized
  // use QStandardPath to find a suitable location for our db file
  QString DBFile = QStandardPaths::locate(QStandardPaths::DocumentsLocation, ProposedFilename);
  bool bNewlyCreated = false;
  if (DBFile.size() > 0)
  {
    // ok, the file is there, we ought to open it
    m_Filename = DBFile;
  }
  else
  {
    // create a suitable filename
    DBFile = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QDir Dir; Dir.mkpath(QDir::toNativeSeparators(DBFile));

    // let's create this path, if needed
    DBFile += "/";
    DBFile += ProposedFilename;
    DBFile = QDir::toNativeSeparators(DBFile);
    bNewlyCreated = true;
  }
  qDebug() << "using DB: " << DBFile;
  DataBase = QSqlDatabase::addDatabase("QSQLITE");
  bool bSuccess = false;
  if (DataBase.isValid())
  {
    DataBase.setDatabaseName(DBFile);
    if (DataBase.open())
    {
      if (PrepareDatabaseForUse(DataBase))
      {
        if (bNewlyCreated)
        {
          bSuccess = CreateTables(DataBase);
        }
        else
        {
          // good, database connection established
          bSuccess = CheckDatabaseForUpdates(DataBase);
        }
      }
    }
  }
  return bSuccess;
}

bool DataBackend_pImpl::CreateTables(QSqlDatabase &DB)
{
  bool bSuccess = true;
  for (auto It = m_Tables.begin(); It != m_Tables.end(); It++)
  {
    if (*It)
    {
      QStringList CreateSQLList = (*It)->getCreateStatements(m_DBVersion);
      for (auto CurCreateSQL = CreateSQLList.begin(); CurCreateSQL != CreateSQLList.end(); CurCreateSQL++)
      {
        QString sCreateSQL = *CurCreateSQL;
        if (sCreateSQL.size() > 0)
        {
          qDebug() << "creating Table using SQL: " << sCreateSQL;
          QSqlQuery Query(DB);
          if (Query.exec(sCreateSQL))
          {
            qDebug() << " ... ok";
          }
          else
          {
            qDebug() << " ... failed!";
            bSuccess = false;
            break;
          }
        }
      }
      if (bSuccess)
      {
        QStringList QueryList = (*It)->insertInitialRows(m_DBVersion);
        for (auto InsertIt = QueryList.cbegin(); InsertIt != QueryList.cend(); InsertIt++)
        {
          qDebug() << "inserting SQL: " << *InsertIt << "...";
          QSqlQuery Query(DB);
          if (Query.exec(*InsertIt))
          {
            qDebug() << " ... ok";
          }
          else
          {
            qDebug() << " ... failed!";
            bSuccess = false;
            break;
          }
        }
      }
      if (!bSuccess)
        break; // do not process other tables on error
    }
  }
  return bSuccess;
}

bool DataBackend_pImpl::CheckDatabaseForUpdates(QSqlDatabase& DB)
{
  bool bSuccess = false;
  // use DB to query our version info
  int ExistingVersion = GetFileDbVersion(DB);
  if (m_DBVersion > ExistingVersion)
  {
    qDebug() << "exisiting DB does not match current DB version";
    qDebug() << " initiating update procedure";
    bSuccess = RunUpdates(DB,ExistingVersion);
  }
  else
  {
    bSuccess = true; // already current version
  }
  return bSuccess;
}

bool DataBackend_pImpl::RunUpdates(QSqlDatabase &DB, int OldVersion)
{
  bool bUpgradesOk = true;
  for (auto It = m_Tables.begin() ; It != m_Tables.end(); It++)
  {
    if ((*It)->NeedUpdate(OldVersion, m_DBVersion))
    {
      QStringList Updates = (*It)->getUpdateStatement(OldVersion, m_DBVersion);
      for (auto UpIt = Updates.cbegin(); UpIt != Updates.cend(); UpIt++)
      {
        QString sUpdate = *(UpIt);
        qDebug() << "Update SQL: " << sUpdate;
        QSqlQuery Query(DB);
        if (Query.exec(sUpdate))
        {
          qDebug() << " ... ok";
        }
        else
        {
          qDebug() << " ... failed !";
          bUpgradesOk = false;
          break;
        }
      }
    }
  }
  return bUpgradesOk;
}

bool DataBackend_pImpl::PrepareDatabaseForUse(QSqlDatabase &DB)
{
  bool bSuccess = false;
  // we want to enable ForeignKeySupport
  QSqlQuery Query(DB);
  if (Query.exec("PRAGMA foreign_keys = ON;"))
  {
    bSuccess = true;
  }
  else
  {
    bSuccess = false;
  }
  return bSuccess;
}

int DataBackend_pImpl::GetFileDbVersion(QSqlDatabase& DB)
{
  int iRet = -1; // indicating error
  // queries the database to read the version
  QSqlQuery Query("select majorversion from dbversion;", DB);
  if (Query.first())
  {
    // qDebug() << "majorversion read: " << Query.value(0);
    iRet = Query.value(0).toInt();
  }
  qDebug() << "read DB Version: "<< iRet;
  return iRet;
}
bool DataBackend_pImpl::DeleteAllData(QSqlDatabase &Db)
{
	bool bDeleteOk = true;
	// we will delete all content on our databases, in reverse order to prevent failure because of foreign key constraints
	for (auto It = m_Tables.rbegin() ; It != m_Tables.rend(); It++)
	{
		QStringList DeleteTableSqlList = (*It)->getDeleteStatements();
		for (auto UpIt = DeleteTableSqlList.cbegin(); UpIt != DeleteTableSqlList.cend(); UpIt++)
		{
			QString sDelete = *(UpIt);
			qDebug() << "Delete SQL: " << sDelete;
			QSqlQuery Query(Db);
			if (Query.exec(sDelete))
			{
				qDebug() << " ... ok";
			}
			else
			{
				qDebug() << " ... failed !";
				bDeleteOk = false;
				break;
			}
		}
	}
	// now insert the default values again
	auto It = m_Tables.begin();
	// however we must not try to insert DB info again
	It++;
	for (;It != m_Tables.end(); It++)
	{
		QStringList InsertStatementList = (*It)->insertInitialRows(m_DBVersion);
		for (auto UpIt = InsertStatementList.cbegin(); UpIt != InsertStatementList.cend(); UpIt++)
		{
			QString sInsert = *(UpIt);
			qDebug() << "Insert SQL: " << sInsert;
			QSqlQuery Query(Db);
			if (Query.exec(sInsert))
			{
				qDebug() << " ... ok";
			}
			else
			{
				qDebug() << " ... failed !";
				bDeleteOk = false;
				break;
			}
		}
	}
	return bDeleteOk;
}

/*******************************************************************
 * DbTableVersion
 * *****************************************************************/
QStringList DbTableVersion::getCreateStatements(int /*TargetVersion*/) const
{
  QStringList CreateList;
  QString sCreate = QString::fromLatin1("CREATE TABLE ");
  sCreate += "dbversion";
  sCreate += " ( majorversion integer primary key)";
  sCreate += ";";
  CreateList.push_back(sCreate);
  return CreateList;
}

bool DbTableVersion::NeedUpdate(int OldVersion, int UpdatedVersion)const
{
  bool bRet = false;
  // we always want to update
  if (UpdatedVersion > OldVersion)
    bRet = true;
  return bRet;
}

QStringList DbTableVersion::getUpdateStatement(int /* OldVersion */, int TargetVersion)const
{
  QString sUpdate = "UPDATE ";
  sUpdate += "dbversion SET ";
  sUpdate += QString("majorversion=%1").arg(TargetVersion);
  sUpdate +=";";
  QStringList sCmdLst;
  sCmdLst.append(sUpdate);
  return sCmdLst;
}

QStringList DbTableVersion::insertInitialRows(int TargetVersion) const
{
  QStringList List;
  QString sInsert = "INSERT INTO dbversion ";
  sInsert += "( majorversion ) VALUES(";
  sInsert += QString("%1").arg(TargetVersion);
  sInsert += ");";
  List.append(sInsert);
  return List;
}

QStringList DbTableVersion::getDeleteStatements() const
{
	QStringList Empty;// DB Version info doesn't get deleted
	return Empty;
}

}
