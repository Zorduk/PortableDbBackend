#include "DbHandlerPrivate.h"

#include <QUuid>

namespace PortableDBBackend
{
	DbHandlerPrivate::DbHandlerPrivate()
	{
		// m_ThreadedDb had its ctor executed and is thus already running its own thread
		initConnections();
	}

	DbHandlerPrivate::~DbHandlerPrivate()
	{
		closeDb();
	}

	void DbHandlerPrivate::initConnections()
	{
		connect(this, &DbHandlerPrivate::threadedSaveToDb, &m_ThreadedDb, &ThreadedDbHandler::onSaveToDb, Qt::QueuedConnection);
		connect(this, &DbHandlerPrivate::threadedUpdateInDb, &m_ThreadedDb, &ThreadedDbHandler::onUpdateInDb, Qt::QueuedConnection);
		connect(this, &DbHandlerPrivate::threadedDeleteInDb, &m_ThreadedDb, &ThreadedDbHandler::onDeleteInDb, Qt::QueuedConnection);
		connect(this, &DbHandlerPrivate::threadedReadFromDb, &m_ThreadedDb, &ThreadedDbHandler::onReadFromDb, Qt::QueuedConnection);
		connect(this, &DbHandlerPrivate::threadedReadAllFromHandler, &m_ThreadedDb, &ThreadedDbHandler::onReadAllFromHandler, Qt::QueuedConnection);
		connect(this, &DbHandlerPrivate::threadedDbVersion, &m_ThreadedDb, &ThreadedDbHandler::onDbVersion, Qt::QueuedConnection);
		connect(this, &DbHandlerPrivate::threadedInitializeDb, &m_ThreadedDb, &ThreadedDbHandler::onInitializeDb, Qt::QueuedConnection);
		connect(this, &DbHandlerPrivate::threadedCloseDb, &m_ThreadedDb, &ThreadedDbHandler::onCloseDb, Qt::QueuedConnection);
		connect(this, &DbHandlerPrivate::threadedDeleteAllInDb, &m_ThreadedDb, &ThreadedDbHandler::onDeleteAllInDb, Qt::QueuedConnection);
		connect(&m_ThreadedDb, &ThreadedDbHandler::DbReady, this, &DbHandlerPrivate::DbReady, Qt::QueuedConnection);
		connect(&m_ThreadedDb, &ThreadedDbHandler::DbReadAllFinishedForHandler, this, &DbHandlerPrivate::DbReadAllFinishedForHandler);
		connect(&m_ThreadedDb, &ThreadedDbHandler::DbError, this, &DbHandlerPrivate::DbError, Qt::QueuedConnection);
	}

	void DbHandlerPrivate::AddTable(std::unique_ptr<ITableDefinition> Table)
	{
		m_ThreadedDb.AddTable(std::move(Table));
	}

	void DbHandlerPrivate::setDbVersion(int DbVersion)
	{
		emit threadedDbVersion(DbVersion, QPrivateSignal());
	}

	void DbHandlerPrivate::InitializeDb(const QString & ProposedFilename)
	{
		emit threadedInitializeDb(ProposedFilename, QPrivateSignal());
	}

	void DbHandlerPrivate::DeleteAllData()
	{
		emit threadedDeleteAllInDb(QPrivateSignal());
	}

	void DbHandlerPrivate::registerHandler(QSharedPointer<DbDataHandlerBase> spHandler)
	{
		if (spHandler)
		{
			QMutexLocker Lock(&m_mHandlerList);
			m_HandlerMap[spHandler->uuid()] = spHandler;
		}
	}

	QSharedPointer<DbDataHandlerBase> DbHandlerPrivate::getHandler(QUuid handlerUuid)
	{
		QSharedPointer<DbDataHandlerBase> spRet;
		{
			QMutexLocker Lock(&m_mHandlerList);
			auto It = m_HandlerMap.find(handlerUuid);
			if (It != m_HandlerMap.end())
			{
				spRet = It->second;
			}
		}
		return spRet;
	}

	void DbHandlerPrivate::saveToDb(QUuid handlerUuid, QVariant value)
	{
		QSharedPointer<DbDataHandlerBase> spHandler = getHandler(handlerUuid);
		if (spHandler)
		{
			emit threadedSaveToDb(spHandler, value, QPrivateSignal());
		}
	}

	void DbHandlerPrivate::updateInDb(QUuid handlerUuid, QVariant value)
	{
		QSharedPointer<DbDataHandlerBase> spHandler = getHandler(handlerUuid);
		if (spHandler)
		{
			emit threadedUpdateInDb(spHandler, value, QPrivateSignal());
		}
	}

	void DbHandlerPrivate::deleteInDb(QUuid handlerUuid, QVariant value)
	{
		QSharedPointer<DbDataHandlerBase> spHandler = getHandler(handlerUuid);
		if (spHandler)
		{
			emit threadedDeleteInDb(spHandler, value, QPrivateSignal());
		}
	}

	void DbHandlerPrivate::readFromDb(QUuid handlerUuid, QVariant value)
	{
		QSharedPointer<DbDataHandlerBase> spHandler = getHandler(handlerUuid);
		if (spHandler)
		{
			emit threadedReadFromDb(spHandler, value, QPrivateSignal());
		}
	}

	void DbHandlerPrivate::readAllFromHandler(QUuid handlerUuid)
	{
		QSharedPointer<DbDataHandlerBase> spHandler = getHandler(handlerUuid);
		if (spHandler)
		{
			emit threadedReadAllFromHandler(spHandler, QPrivateSignal());
		}
	}

	void DbHandlerPrivate::readAll()
	{
		// I rather want to create a copy of all QtSharedPointer
		std::vector<QSharedPointer<DbDataHandlerBase>> handlerList;
		{
			QMutexLocker Lock(&m_mHandlerList);
			for (auto HandlerIt : m_HandlerMap)
			{
				handlerList.push_back(HandlerIt.second);
			}
		}
		// leave scope to release QMutex
		for (auto& spHandler : handlerList)
		{
			emit threadedReadAllFromHandler(spHandler, QPrivateSignal());
		}
	}

	void DbHandlerPrivate::closeDb()
	{
		emit threadedCloseDb(QPrivateSignal());
	}

	/**********************************************************
	*	ThreadedDbHandler
	***********************************************************/
	ThreadedDbHandler::ThreadedDbHandler()
	{

	}

	ThreadedDbHandler::~ThreadedDbHandler()
	{
		finishThread();
	}

	void ThreadedDbHandler::AddTable(std::unique_ptr<ITableDefinition> Table)
	{
		QMutexLocker Lock(&m_mDatabaseDefinition);
		m_DbManager.AddTable(std::move(Table));
	}

	void ThreadedDbHandler::onSaveToDb(QSharedPointer<DbDataHandlerBase> spHandler, QVariant Value)
	{
		if (spHandler)
		{
			spHandler->saveToDb(Value, m_Db);
		}
	}

	void ThreadedDbHandler::onUpdateInDb(QSharedPointer<DbDataHandlerBase> spHandler, QVariant Value)
	{
		if (spHandler)
		{
			spHandler->updateInDb(Value, m_Db);
		}
	}

	void ThreadedDbHandler::onDeleteInDb(QSharedPointer<DbDataHandlerBase> spHandler, QVariant Value)
	{
		if (spHandler)
		{
			spHandler->deleteInDb(Value, m_Db);
		}
	}

	void ThreadedDbHandler::onReadFromDb(QSharedPointer<DbDataHandlerBase> spHandler, QVariant Value)
	{
		if (spHandler)
		{
			spHandler->readFromDb(Value, m_Db);
		}
	}

	void ThreadedDbHandler::onReadAllFromHandler(QSharedPointer<DbDataHandlerBase> spHandler)
	{
		if (spHandler)
		{
			spHandler->readAll(m_Db);
			emit DbReadAllFinishedForHandler(spHandler->uuid());
		}
	}

	void ThreadedDbHandler::onDeleteAllInDb()
	{
		m_DbManager.DeleteAllData(m_Db);
	}

	void ThreadedDbHandler::onDbVersion(int DbVersion)
	{
		QMutexLocker Lock(&m_mDatabaseDefinition);
		m_DbManager.setDbVersion(DbVersion);
	}

	void ThreadedDbHandler::onInitializeDb(const QString & ProposedFilename)
	{
		QMutexLocker Lock(&m_mDatabaseDefinition);
		if (m_DbManager.InitializeDB(ProposedFilename, m_Db))
		{
			emit DbReady();
		}
	}

	void ThreadedDbHandler::onCloseDb()
	{
		m_Db.close();
	}

	void ThreadedDbHandler::onShutDown()
	{
		m_DbThread.quit();
	}

	void ThreadedDbHandler::onThreadedInit()
	{
		// nothing yet :)
	}

	void ThreadedDbHandler::finishThread()
	{
		// we signal our shutdown event in order to still execute other events still in the queue
		// which would be disregarded on m_DbThread.quit()
		emit shutdownDbHandler(QPrivateSignal());

		// now wait until the thread finishes execution
		// there is no proper reaction to a timeout - a terminate is not guaranteed to work and could possibly corrupt save data
		// so we better wait regardless of how long it takes
		m_DbThread.wait();
	}

	void ThreadedDbHandler::initializeThread()
	{
		moveToThread(&m_DbThread);
		// make some vital connections
		connect(this, &ThreadedDbHandler::threadedInit, this, &ThreadedDbHandler::onThreadedInit, Qt::QueuedConnection);
		connect(this, &ThreadedDbHandler::shutdownDbHandler, this, &ThreadedDbHandler::onShutDown, Qt::QueuedConnection);
		// now start the thread
		m_DbThread.start();
		emit threadedInit(QPrivateSignal());
	}
}