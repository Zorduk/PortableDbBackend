#include "databackend.h"
#include "databackend_pimpl.h"

#include <thread>
#include <chrono>

namespace PortableDBBackend
{

DataBackend::DataBackend()
  : QObject(nullptr),
    m_spPImpl(new DataBackend_pImpl)
{
}

DataBackend::~DataBackend()
{
}

bool DataBackend::InitializeDB(const QString& ProposedFilename, QSqlDatabase& DbToInitialize)
{
  bool bSuccess = m_spPImpl->InitializeDB(ProposedFilename, DbToInitialize);
  return bSuccess;
}

void DataBackend::setDbVersion(int CurrentVersion)
{
  m_spPImpl->setDbVersion(CurrentVersion);
}

void DataBackend::AddTable(std::unique_ptr<ITableDefinition> Table)
{
  m_spPImpl->AddTable(std::move(Table));
}

bool DataBackend::DeleteAllData(QSqlDatabase& DbToDelete)
{
	return m_spPImpl->DeleteAllData(DbToDelete);
}

}
