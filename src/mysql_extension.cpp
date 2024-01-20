#define DUCKDB_BUILD_LOADABLE_EXTENSION
#include "duckdb.hpp"

#include "mysql_scanner.hpp"
#include "mysql_storage.hpp"
#include "mysql_scanner_extension.hpp"

#include "duckdb/catalog/catalog.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/main/database_manager.hpp"
#include "duckdb/main/attached_database.hpp"
#include "storage/mysql_catalog.hpp"
#include "storage/mysql_optimizer.hpp"

using namespace duckdb;

static void SetMySQLDebugQueryPrint(ClientContext &context, SetScope scope, Value &parameter) {
	MySQLConnection::DebugSetPrintQueries(BooleanValue::Get(parameter));
}

static void LoadInternal(DatabaseInstance &db) {
	mysql_library_init(0, NULL, NULL);
	MySQLClearCacheFunction clear_cache_func;
	ExtensionUtil::RegisterFunction(db, clear_cache_func);

	auto &config = DBConfig::GetConfig(db);
	config.storage_extensions["mysql_scanner"] = make_uniq<MySQLStorageExtension>();

	config.AddExtensionOption("mysql_experimental_filter_pushdown",
	                          "Whether or not to use filter pushdown (currently experimental)", LogicalType::BOOLEAN,
	                          Value::BOOLEAN(false));
	config.AddExtensionOption("mysql_debug_show_queries", "DEBUG SETTING: print all queries sent to MySQL to stdout",
	                          LogicalType::BOOLEAN, Value::BOOLEAN(false), SetMySQLDebugQueryPrint);
	config.AddExtensionOption("mysql_tinyint1_as_boolean", "Whether or not to convert TINYINT(1) columns to BOOLEAN",
	                          LogicalType::BOOLEAN, Value::BOOLEAN(true), MySQLClearCacheFunction::ClearCacheOnSetting);
	config.AddExtensionOption("mysql_bit1_as_boolean", "Whether or not to convert BIT(1) columns to BOOLEAN",
	                          LogicalType::BOOLEAN, Value::BOOLEAN(true), MySQLClearCacheFunction::ClearCacheOnSetting);

	OptimizerExtension mysql_optimizer;
	mysql_optimizer.optimize_function = MySQLOptimizer::Optimize;
	config.optimizer_extensions.push_back(std::move(mysql_optimizer));
}

void MySQLScannerExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}

extern "C" {

DUCKDB_EXTENSION_API void mysql_scanner_init(duckdb::DatabaseInstance &db) {
	LoadInternal(db);
}

DUCKDB_EXTENSION_API const char *mysql_scanner_version() {
	return DuckDB::LibraryVersion();
}

DUCKDB_EXTENSION_API void mysql_scanner_storage_init(DBConfig &config) {
	mysql_library_init(0, NULL, NULL);
	config.storage_extensions["mysql_scanner"] = make_uniq<MySQLStorageExtension>();
}
}
