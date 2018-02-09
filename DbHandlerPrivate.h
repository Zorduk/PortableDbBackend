#pragma once
#include "DbHandler.h"
#include "databackend.h"

#include <QMutex>
#include <QThread>

namespace PortableDBBackend
{
	//! @brief ThreadedDbHandler runs all its slots in its own thread and is the only class with access to the database
	class ThreadedDbHandler : public QObject
	{
		Q_OBJECT
	public:
		ThreadedDbHandler();
		virtual ~ThreadedDbHandler();

		//! add definition for database tables
		//! unfortunately the unique_ptr design prevents us from using signal/slot queued connections
		//! thus we will directly call the embedded ThreadedDbHandler in which AddTable is secured by a mutex
		void AddTable(std::unique_ptr<ITableDefinition> Table);

	public slots:
		void onSaveToDb(QSharedPointer<DbDataHandlerBase> spHandler, QVariant Value);
		void onUpdateInDb(QSharedPointer<DbDataHandlerBase> spHandler, QVariant Value);
		void onDeleteInDb(QSharedPointer<DbDataHandlerBase> spHandler, QVariant Value);
		void onReadFromDb(QSharedPointer<DbDataHandlerBase> spHandler, QVariant Value);
		void onReadAllFromHandler(QSharedPointer<DbDataHandlerBase> spHandler);
		void onDeleteAllInDb();
		void onDbVersion(int DbVersion);
		void onInitializeDb(const QString& ProposedFilename);
		void onCloseDb();

	signals:
		void DbError(const QString& ErrorDsc, DbErrorCode ErrorCode);
		void threadedInit(QPrivateSignal);
		void shutdownDbHandler(QPrivateSignal);
		void DbReady();
		void DbReadAllFinishedForHandler(QUuid handlerUuid);

	private slots:
		void onThreadedInit();
		void onShutDown();

	private:
		QMutex m_mDatabaseDefinition; //!< protect/serialize m_DbManager calls
		DataBackend m_DbManager;
		QSqlDatabase m_Db;
		QThread m_DbThread;

		void finishThread(); //!< called from d'tor in main thread, initiate shutdown and wait until thread is terminated
		void initializeThread();//!< called from ctor in main thread, will move this QObject to its own thread

	};

	//! @brief DbHandlerPrivate contains the ThreadedDbHandler 
	//! and uses QueuedConnections to communicate
	class DbHandlerPrivate : public QObject
	{
		Q_OBJECT
	public:
		DbHandlerPrivate();
		virtual ~DbHandlerPrivate();

		// Database definition
		//! add definition for database tables
		//! unfortunately the unique_ptr design prevents us from using signal/slot queued connections
		//! thus we will directly call the embedded ThreadedDbHandler in which AddTable is secured by a mutex
		void AddTable(std::unique_ptr<ITableDefinition> Table);

		//! set the current Db schema version
		void setDbVersion(int DbVersion);

		void InitializeDb(const QString& ProposedFilename);
		void DeleteAllData();

		void registerHandler(QSharedPointer<DbDataHandlerBase> spHandler);

		//! @brief will return nullptr for unknown Uuids
		QSharedPointer<DbDataHandlerBase> getHandler(QUuid handlerUuid);

	public slots:
		void saveToDb(QUuid handlerUuid, QVariant value);
		void updateInDb(QUuid handlerUuid, QVariant value);
		void deleteInDb(QUuid handlerUuid, QVariant value);
		void readFromDb(QUuid handlerUuid, QVariant value);
		void readAllFromHandler(QUuid handlerUuid);
		void readAll(); //!< will query trigger all registered handlers to read their values
		void closeDb();

	signals:
		void DbError(const QString& ErrorDsc, DbErrorCode ErrorCode);
		void DbReady();
		void DbReadAllFinishedForHandler(QUuid handlerUuid);


		// signals to communicate with ThreadedDb (using QueuedConnections)
		void threadedSaveToDb(QSharedPointer<DbDataHandlerBase> spHandler, QVariant Value, QPrivateSignal);
		void threadedUpdateInDb(QSharedPointer<DbDataHandlerBase> spHandler, QVariant Value, QPrivateSignal);
		void threadedDeleteInDb(QSharedPointer<DbDataHandlerBase> spHandler, QVariant Value, QPrivateSignal);
		void threadedReadFromDb(QSharedPointer<DbDataHandlerBase> spHandler, QVariant Value, QPrivateSignal);
		void threadedReadAllFromHandler(QSharedPointer<DbDataHandlerBase> spHandler, QPrivateSignal);
		void threadedDbVersion(int DbSchemaVersion, QPrivateSignal);
		void threadedInitializeDb(const QString& ProposedFilename, QPrivateSignal);
		void threadedCloseDb(QPrivateSignal);
		void threadedDeleteAllInDb(QPrivateSignal);

	private:
		ThreadedDbHandler m_ThreadedDb;
		QMutex m_mHandlerList;
		std::map<QUuid, QSharedPointer<DbDataHandlerBase> > m_HandlerMap;

		void initConnections(); //!< called from ctor to create all the needed connections
	};

}