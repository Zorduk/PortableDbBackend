#include "DbHandler.h"
#include "DbHandlerPrivate.h"



namespace PortableDBBackend
{

	DbHandler::DbHandler()
		: m_pImpl(std::make_unique<DbHandlerPrivate>())
	{
		connect(m_pImpl.get(), &DbHandlerPrivate::DbError, this, &DbHandler::DbError);
		connect(m_pImpl.get(), &DbHandlerPrivate::DbReady, this, &DbHandler::DbReady);
		connect(m_pImpl.get(), &DbHandlerPrivate::DbReadAllFinishedForHandler, this, &DbHandler::DbReadAllFinishedForHandler);
	}

	DbHandler::~DbHandler()
	{
	}

	void DbHandler::AddTable(std::unique_ptr<ITableDefinition> Table)
	{
		m_pImpl->AddTable(std::move(Table));
	}

	void DbHandler::setDbVersion(int DbVersion)
	{
		m_pImpl->setDbVersion(DbVersion);
	}

	void DbHandler::InitializeDb(const QString & ProposedFilename)
	{
		m_pImpl->InitializeDb(ProposedFilename);
	}

	void DbHandler::DeleteAllData()
	{
		m_pImpl->DeleteAllData();
	}

	void DbHandler::closeDb()
	{
		m_pImpl->closeDb();
	}

	void DbHandler::registerHandler(QSharedPointer<DbDataHandlerBase> spHandler)
	{
		m_pImpl->registerHandler(spHandler);
	}

	QSharedPointer<DbDataHandlerBase> DbHandler::getHandler(QUuid handlerUuid)
	{
		return m_pImpl->getHandler(handlerUuid);
	}

	void DbHandler::updateInDb(QUuid handlerUuid, QVariant value)
	{
		m_pImpl->updateInDb(handlerUuid, value);
	}

	void DbHandler::deleteInDb(QUuid handlerUuid, QVariant value)
	{
		m_pImpl->deleteInDb(handlerUuid, value);
	}

	void DbHandler::readFromDb(QUuid handlerUuid, QVariant value)
	{
		m_pImpl->readFromDb(handlerUuid, value);
	}

	void DbHandler::readAllFromHandler(QUuid handlerUuid)
	{
		m_pImpl->readAllFromHandler(handlerUuid);
	}

	void DbHandler::readAll()
	{
		m_pImpl->readAll();
	}

	void DbHandler::saveToDb(QUuid handlerUuid, QVariant value)
	{
		m_pImpl->saveToDb(handlerUuid, value);
	}
}