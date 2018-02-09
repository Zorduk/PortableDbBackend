#pragma once

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QUuid>

#include "databackend.h"

#include <memory>

namespace PortableDBBackend
{

	enum class DbErrorCode
	{
		Success = 0,
		General = 1
	};

	class DbHandlerPrivate;

	//! @brief DbDataHandlerBase defines the interface for database handlers
	class DbDataHandlerBase  : public QObject
	{
		Q_OBJECT
	public:
		DbDataHandlerBase() = default;
		virtual ~DbDataHandlerBase() = default;
				
		// return Uuid of this handler
		virtual QUuid uuid() const = 0;
	
		// register/install = database signals
		//! @brief databaseOpened is called for all handlers when the database was opened or a new handler is registered for an already opened database
		virtual void databaseOpened(QSqlDatabase& /*Db*/) {};
		//! @ brief databaseClosed is called when the current database was closed
		//! Handlers should invalidate any prepared queries or cached data at this point
		virtual void databaseClosed() {};

		// operation
		virtual void saveToDb(QVariant /*value*/, QSqlDatabase& /*Db*/) {};
		virtual void updateInDb(QVariant /*value*/, QSqlDatabase& /*Db*/) {};
		virtual void deleteInDb(QVariant /*value*/, QSqlDatabase& /*Db*/) {};
		virtual void readFromDb(QVariant /*value*/, QSqlDatabase& /*Db*/) {};
		virtual void readAll(QSqlDatabase& /*Db*/) {};

	signals:
		void DbError(const QString& ErrorDsc, DbErrorCode ErrorCode);
	};

	//! @brief DbHandler provides an interface to Db which runs in its own thread
	//! interthread communication is done with Qt signals/slots
	//! therefor all data that is passed along must be known to the Qt metasystem to allow queued connections
	//! DbHandler expects all AddTable calls before the database is actually opened
	//! DbHandler expects registerDbHandler() calls before any data is exchanged, can be set even if the database isn't yet initialized
	class DbHandler : public QObject
	{
		Q_OBJECT
	public:
		DbHandler();
		virtual ~DbHandler();

		// Database definition
		//! add definition for database tables
		template <class TableType> void addTableType()
		{
			AddTable(std::unique_ptr<ITableDefinition>(new TableType));
		}
		void AddTable(std::unique_ptr<ITableDefinition> Table);
		
		//! set the current Db schema version
		void setDbVersion(int DbVersion);

		void InitializeDb(const QString& ProposedFilename);
		void DeleteAllData();
		void closeDb();

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

	signals:
		void DbError(const QString& ErrorDsc, DbErrorCode ErrorCode);
		void DbReady();
		void DbReadAllFinishedForHandler(QUuid handlerUuid); //  indicates that the readAll function from this handler has reported all its data

	private:
		std::unique_ptr<DbHandlerPrivate> m_pImpl;
	};

}
Q_DECLARE_METATYPE(QSharedPointer<PortableDBBackend::DbDataHandlerBase>);