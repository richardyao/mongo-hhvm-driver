// Copyright (c) 2014. All rights reserved.

#include "stringprintf.h"
#include "mcon/types.h"
#include "mcon/parse.h"
#include "mcon/manager.h"
#include "ext_mongo.h"
#include "io_stream.h"
#include "log.h"

namespace HPHP {

const StaticString 
    s_Mongo("Mongo"),
    s_manager("__manager"),
    s_servers("__servers");

//////////////////////////////////////////////////////////////////////////////
// class Mongo

static bool HHVM_METHOD(Mongo, connectUtil) {
  throw_not_implemented("Mongo::connectUtil");
}

static int64_t HHVM_STATIC_METHOD(Mongo, getPoolSize) {
  throw_not_implemented("Mongo::getPoolSize");
}

static String HHVM_METHOD(Mongo, getSlave) {
  throw_not_implemented("Mongo::getSlave");
}

static bool HHVM_METHOD(Mongo, getSlaveOkay) {
  throw_not_implemented("Mongo::getSlaveOkay");
}

static Array HHVM_METHOD(Mongo, poolDebug) {
  throw_not_implemented("Mongo::poolDebug");
}

static bool HHVM_STATIC_METHOD(Mongo, setPoolSize, int64_t size) {
  throw_not_implemented("Mongo::setPoolSize");
}

static bool HHVM_METHOD(Mongo, setSlaveOkay, bool ok) {
  throw_not_implemented("Mongo::setSlaveOkay");
}

static String HHVM_METHOD(Mongo, switchSlave) {
  throw_not_implemented("Mongo::switchSlave");
}

const StaticString s_MongoBinData("MongoBinData");
//////////////////////////////////////////////////////////////////////////////
// class MongoBinData

static void HHVM_METHOD(MongoBinData, __construct, const String& data, int64_t type) {
  throw_not_implemented("MongoBinData::__construct");
}

static String HHVM_METHOD(MongoBinData, __toString) {
  throw_not_implemented("MongoBinData::__toString");
}

const StaticString s_MongoClient("MongoClient");
//////////////////////////////////////////////////////////////////////////////
// class MongoClient


/* {{{ Helper for connecting the servers */
mongo_connection *php_mongo_connect(mongo_con_manager *manager, 
                                    mongo_servers *servers, 
                                    int flags)
{
    mongo_connection *con;
    char *error_message = NULL;

    /* We don't care about the result so although we assign it to a var, we
     * only do that to handle errors and return it so that the calling function
     * knows whether a connection could be obtained or not. */
    con = mongo_get_read_write_connection(manager, servers, flags, (char **)&error_message);
    if (con) {
        return con;
    }

    if (error_message) {
        Array* params = new Array();
        params->append(Variant(71));
        params->append(Variant(String(error_message)));
        free(error_message);
        Object e = create_object("MongoConnectionException", *params, true);
        throw e;
    } else {
        Array* params = new Array();
        params->append(Variant(72));
        params->append(Variant(String("Unknown error obtaining connection")));
        free(error_message);
        Object e = create_object("MongoConnectionException", *params, true);
        throw e;
    }
    return NULL;
}
/* }}} */


/* {{{ Helper for special options, that can't be represented by a simple key
 * value pair, or options that are not actually connection string options. */
int mongo_store_option_wrapper(mongo_con_manager *manager, 
                               mongo_servers *servers, 
                               String option_name, 
                               Variant option_value, 
                               char **error_message)
{
    /* Special cases:
     *  - "connect" isn't supported by the URL parsing
     *  - "readPreferenceTags" is an array of tagsets we need to iterate over
     */
    if (option_name == String("connect")) {
        return 4;
    }
    if (option_name == String("readPreferenceTags")) {
        int            error = 0;

        for (ArrayIter iter(option_value.toArray()); iter; ++iter) {
            Variant opt_entry = iter.second();
            Variant opt_key = iter.first();

            error = mongo_store_option(manager, servers, option_name.c_str(), opt_entry.toString().c_str(), error_message);
            if (error) {
                return error;
            }
        }
        return error;
    }

    return mongo_store_option(manager, servers, option_name.c_str(), option_value.toString().c_str(), error_message);
}
/* }}} */


static void HHVM_METHOD(MongoClient, __construct, 
                        const String& server, 
                        const Array& options, 
                        const Array& driver_options) 
{
    bool connect;
    int error;
    char *error_message = NULL;

    /* Set the manager from the global manager */
    mongo_con_manager *manager = s_mongo_extension.manager_;

    /* Parse the server specification
     * Default to the mongo.default_host & mongo.default_port INI options */
    mongo_servers *servers = mongo_parse_init();
    if (!server.empty()) {
        error = mongo_parse_server_spec(manager, servers, server.c_str(), (char **)&error_message);
        if (error) {
            Array* params = new Array();
            params->append(Variant(20 + error));
            params->append(Variant(String(error_message)));
            free(error_message);
            Object e = create_object("MongoConnectionException", *params, true);
            throw e;
        }
    } else {
        std::string tmp;

        tmp = StringPrintf("%s:%ld", s_mongo_extension.default_host_, s_mongo_extension.default_port_);
        error = mongo_parse_server_spec(manager, servers, tmp.c_str(), (char **)&error_message);

        if (error) {
            Array* params = new Array();
            params->append(Variant(0));
            params->append(Variant(String(error_message)));
            free(error_message);
            Object e = create_object("MongoConnectionException", *params, true);
            throw e;
        } 
    }

    /* If "w" was *not* set as an option, then assign the default */
    if (servers->options.default_w == -1 && servers->options.default_wstring == NULL) {
        /* Default to WriteConcern=1 for MongoClient */
        servers->options.default_w = 1;
    }

    /* Options through array */
    for (ArrayIter iter(options); iter; ++iter) {
        Variant opt_entry = iter.second();
        Variant opt_key = iter.first();

        switch (opt_key.getType()) {
        case KindOfString:
        case KindOfStaticString: {
            int error_code = 0;

            error_code = mongo_store_option_wrapper(manager, servers, opt_key.toString(), opt_entry, (char **)&error_message);

            switch (error_code) {
            case -1: /* Deprecated options */
                if (opt_key.toString() == String("slaveOkay")) {
                    raise_deprecated("The 'slaveOkay' option is deprecated. Please switch to read-preferences");
                } else if (opt_key.toString() == String("timeout")) {
                    raise_deprecated("The 'timeout' option is deprecated. Please use 'connectTimeoutMS' instead");
                }
                break;
            case 4: /* Special options parameters, invalid for URL parsing - only possiblity is 'connect' for now */
                if (opt_key.toString() == String("connect")) {
                    connect = opt_entry.toBoolean();
                }
                break;

            case 3: /* Logical error (i.e. conflicting options) */
            case 2: /* Unknown connection string option */
            case 1: /* Empty option name or value */
                /* Throw exception - error code is 20 + above value. They are defined in php_mongo.h */
                Array* params = new Array();
                params->append(Variant(20 + error_code));
                params->append(Variant(String(error_message)));
                free(error_message);
                Object e = create_object("MongoConnectionException", *params, true);
                throw e;
            }
        } break;
        case KindOfInt64: {
            /* Throw exception - error code is 25. This is defined in php_mongo.h */
            Array* params = new Array();
            params->append(Variant(25));
            params->append(Variant(String("Unrecognized or unsupported option")));
            Object e = create_object("MongoConnectionException", *params, true);
            throw e;
        } break;
        default:
            break;
        }
    }

    {
        // TODO(stream_context)
    }

    // TODO

    if (connect) {
        /* Make sure we clear any exceptions thrown if have any usable connection */
        if (php_mongo_connect(manager, servers, MONGO_CON_FLAG_READ|MONGO_CON_FLAG_DONT_FILTER)) {
            //zend_clear_exception(TSRMLS_C);
        }
    }

    this_->o_set(s_manager, manager, s_MongoClient.get());
    this_->o_set(s_servers, servers, s_MongoClient.get());
}

static bool HHVM_METHOD(MongoClient, close, const Object& connection) {
  throw_not_implemented("MongoClient::close");
}

static bool HHVM_METHOD(MongoClient, connect) {
  throw_not_implemented("MongoClient::connect");
}

static Array HHVM_METHOD(MongoClient, dropDB, const Variant& db) {
  throw_not_implemented("MongoClient::dropDB");
}

static Object HHVM_METHOD(MongoClient, __get, const String& dbname) {
  throw_not_implemented("MongoClient::__get");
}

static Array HHVM_STATIC_METHOD(MongoClient, getConnections) {
  throw_not_implemented("MongoClient::getConnections");
}

static Array HHVM_METHOD(MongoClient, getHosts) {
  throw_not_implemented("MongoClient::getHosts");
}

static Array HHVM_METHOD(MongoClient, getReadPreference) {
  throw_not_implemented("MongoClient::getReadPreference");
}

static Array HHVM_METHOD(MongoClient, getWriteConcern) {
  throw_not_implemented("MongoClient::getWriteConcern");
}

static bool HHVM_METHOD(MongoClient, killCursor, const String& server_hash, const Object& id) {
  throw_not_implemented("MongoClient::killCursor");
}

static Array HHVM_METHOD(MongoClient, listDBs) {
  throw_not_implemented("MongoClient::listDBs");
}

static Object HHVM_METHOD(MongoClient, selectCollection, const String& db, const String& collection) {
  throw_not_implemented("MongoClient::selectCollection");
}

static Object HHVM_METHOD(MongoClient, selectDB, const String& name) {
  throw_not_implemented("MongoClient::selectDB");
}

static bool HHVM_METHOD(MongoClient, setReadPreference, const String& read_preference, const Array& tags) {
  throw_not_implemented("MongoClient::setReadPreference");
}

static bool HHVM_METHOD(MongoClient, setWriteConcern, const Variant& w, int64_t wtimeout) {
  throw_not_implemented("MongoClient::setWriteConcern");
}

static String HHVM_METHOD(MongoClient, __toString) {
  throw_not_implemented("MongoClient::__toString");
}

const StaticString s_MongoCode("MongoCode");
//////////////////////////////////////////////////////////////////////////////
// class MongoCode

static void HHVM_METHOD(MongoCode, __construct, const String& code, const Array& scope) {
  throw_not_implemented("MongoCode::__construct");
}

static String HHVM_METHOD(MongoCode, __toString) {
  throw_not_implemented("MongoCode::__toString");
}

const StaticString s_MongoCollection("MongoCollection");
//////////////////////////////////////////////////////////////////////////////
// class MongoCollection

static Array HHVM_METHOD(MongoCollection, aggregate, const Array& op, const Array& ...) {
  throw_not_implemented("MongoCollection::aggregate");
}

static Object HHVM_METHOD(MongoCollection, aggregateCursor, const Array& command, const Array& options) {
  throw_not_implemented("MongoCollection::aggregateCursor");
}

static Variant HHVM_METHOD(MongoCollection, batchInsert, const Array& a, const Array& options) {
  throw_not_implemented("MongoCollection::batchInsert");
}

static void HHVM_METHOD(MongoCollection, __construct, const Object& db, const String& name) {
  throw_not_implemented("MongoCollection::__construct");
}

static int64_t HHVM_METHOD(MongoCollection, count, const Array& query, int64_t limit, int64_t skip) {
  throw_not_implemented("MongoCollection::count");
}

static Array HHVM_METHOD(MongoCollection, createDBRef, const Variant& document_or_id) {
  throw_not_implemented("MongoCollection::createDBRef");
}

static bool HHVM_METHOD(MongoCollection, createIndex, const Array& keys, const Array& options) {
  throw_not_implemented("MongoCollection::createIndex");
}

static Array HHVM_METHOD(MongoCollection, deleteIndex, const Object& keys) {
  throw_not_implemented("MongoCollection::deleteIndex");
}

static Array HHVM_METHOD(MongoCollection, deleteIndexes) {
  throw_not_implemented("MongoCollection::deleteIndexes");
}

static Array HHVM_METHOD(MongoCollection, distinct, const String& key, const Array& query) {
  throw_not_implemented("MongoCollection::distinct");
}

static Array HHVM_METHOD(MongoCollection, drop) {
  throw_not_implemented("MongoCollection::drop");
}

//static bool HHVM_METHOD(MongoCollection, ensureIndex, const Object& key|keys, const Array& options) {
static bool HHVM_METHOD(MongoCollection, ensureIndex, const Variant& key, const Array& options) {
  throw_not_implemented("MongoCollection::ensureIndex");
}

static Object HHVM_METHOD(MongoCollection, find, const Array& query, const Array& fields) {
  throw_not_implemented("MongoCollection::find");
}

static Array HHVM_METHOD(MongoCollection, findAndModify, const Array& query, const Array& update, const Array& fields, const Array& options) {
  throw_not_implemented("MongoCollection::findAndModify");
}

static Array HHVM_METHOD(MongoCollection, findOne, const Array& query, const Array& fields, const Array& options) {
  throw_not_implemented("MongoCollection::findOne");
}

static Object HHVM_METHOD(MongoCollection, __get, const String& name) {
  throw_not_implemented("MongoCollection::__get");
}

static Array HHVM_METHOD(MongoCollection, getDBRef, const Array& ref) {
  throw_not_implemented("MongoCollection::getDBRef");
}

static Array HHVM_METHOD(MongoCollection, getIndexInfo) {
  throw_not_implemented("MongoCollection::getIndexInfo");
}

static String HHVM_METHOD(MongoCollection, getName) {
  throw_not_implemented("MongoCollection::getName");
}

static Array HHVM_METHOD(MongoCollection, getReadPreference) {
  throw_not_implemented("MongoCollection::getReadPreference");
}

static bool HHVM_METHOD(MongoCollection, getSlaveOkay) {
  throw_not_implemented("MongoCollection::getSlaveOkay");
}

static Array HHVM_METHOD(MongoCollection, getWriteConcern) {
  throw_not_implemented("MongoCollection::getWriteConcern");
}

static Array HHVM_METHOD(MongoCollection, group, const Variant& keys, const Array& initial, const Object& reduce, const Array& options) {
  throw_not_implemented("MongoCollection::group");
}

static Object HHVM_METHOD(MongoCollection, insert, const Object& a, const Array& options) {
  throw_not_implemented("MongoCollection::insert");
}

static Object HHVM_METHOD(MongoCollection, parallelCollectionScan, int64_t num_cursors) {
  throw_not_implemented("MongoCollection::parallelCollectionScan");
}

static Object HHVM_METHOD(MongoCollection, remove, const Array& criteria, const Array& options) {
  throw_not_implemented("MongoCollection::remove");
}

static Variant HHVM_METHOD(MongoCollection, save, const Object& a, const Array& options) {
  throw_not_implemented("MongoCollection::save");
}

static bool HHVM_METHOD(MongoCollection, setReadPreference, const String& read_preference, const Array& tags) {
  throw_not_implemented("MongoCollection::setReadPreference");
}

static bool HHVM_METHOD(MongoCollection, setSlaveOkay, bool ok) {
  throw_not_implemented("MongoCollection::setSlaveOkay");
}

static bool HHVM_METHOD(MongoCollection, setWriteConcern, const Variant& w, int64_t wtimeout) {
  throw_not_implemented("MongoCollection::setWriteConcern");
}

static String HHVM_METHOD(MongoCollection, toIndexString, const Variant& keys) {
  throw_not_implemented("MongoCollection::toIndexString");
}

static String HHVM_METHOD(MongoCollection, __toString) {
  throw_not_implemented("MongoCollection::__toString");
}

static Object HHVM_METHOD(MongoCollection, update, const Array& criteria, const Array& new_object, const Array& options) {
  throw_not_implemented("MongoCollection::update");
}

static Array HHVM_METHOD(MongoCollection, validate, bool scan_data) {
  throw_not_implemented("MongoCollection::validate");
}

const StaticString s_MongoCommandCursor("MongoCommandCursor");
//////////////////////////////////////////////////////////////////////////////
// class MongoCommandCursor

static Object HHVM_METHOD(MongoCommandCursor, batchSize, int64_t batchSize) {
  throw_not_implemented("MongoCommandCursor::batchSize");
}

static void HHVM_METHOD(MongoCommandCursor, __construct, const Object& connection, const String& ns, const Array& command) {
  throw_not_implemented("MongoCommandCursor::__construct");
}

static void HHVM_METHOD(MongoCommandCursor, createFromDocument, const Object& connection, const String& hash, const Array& document) {
  throw_not_implemented("MongoCommandCursor::createFromDocument");
}

static Array HHVM_METHOD(MongoCommandCursor, current) {
  throw_not_implemented("MongoCommandCursor::current");
}

static bool HHVM_METHOD(MongoCommandCursor, dead) {
  throw_not_implemented("MongoCommandCursor::dead");
}

static Array HHVM_METHOD(MongoCommandCursor, info) {
  throw_not_implemented("MongoCommandCursor::info");
}

static String HHVM_METHOD(MongoCommandCursor, key) {
  throw_not_implemented("MongoCommandCursor::key");
}

static void HHVM_METHOD(MongoCommandCursor, next) {
  throw_not_implemented("MongoCommandCursor::next");
}

static Array HHVM_METHOD(MongoCommandCursor, rewind) {
  throw_not_implemented("MongoCommandCursor::rewind");
}

static bool HHVM_METHOD(MongoCommandCursor, valid) {
  throw_not_implemented("MongoCommandCursor::valid");
}

const StaticString s_MongoConnectionException("MongoConnectionException");
const StaticString s_MongoCursor("MongoCursor");
//////////////////////////////////////////////////////////////////////////////
// class MongoCursor

static Object HHVM_METHOD(MongoCursor, addOption, const String& key, const Variant& value) {
  throw_not_implemented("MongoCursor::addOption");
}

static Object HHVM_METHOD(MongoCursor, awaitData, bool wait) {
  throw_not_implemented("MongoCursor::awaitData");
}

static Object HHVM_METHOD(MongoCursor, batchSize, int64_t batchSize) {
  throw_not_implemented("MongoCursor::batchSize");
}

static void HHVM_METHOD(MongoCursor, __construct, const Object& connection, const String& ns, const Array& query, const Array& fields) {
  throw_not_implemented("MongoCursor::__construct");
}

static int64_t HHVM_METHOD(MongoCursor, count, bool foundOnly) {
  throw_not_implemented("MongoCursor::count");
}

static Array HHVM_METHOD(MongoCursor, current) {
  throw_not_implemented("MongoCursor::current");
}

static bool HHVM_METHOD(MongoCursor, dead) {
  throw_not_implemented("MongoCursor::dead");
}

static void HHVM_METHOD(MongoCursor, doQuery) {
  throw_not_implemented("MongoCursor::doQuery");
}

static Array HHVM_METHOD(MongoCursor, explain) {
  throw_not_implemented("MongoCursor::explain");
}

static Object HHVM_METHOD(MongoCursor, fields, const Array& f) {
  throw_not_implemented("MongoCursor::fields");
}

static Array HHVM_METHOD(MongoCursor, getNext) {
  throw_not_implemented("MongoCursor::getNext");
}

static Array HHVM_METHOD(MongoCursor, getReadPreference) {
  throw_not_implemented("MongoCursor::getReadPreference");
}

static bool HHVM_METHOD(MongoCursor, hasNext) {
  throw_not_implemented("MongoCursor::hasNext");
}

static Object HHVM_METHOD(MongoCursor, hint, const Variant& index) {
  throw_not_implemented("MongoCursor::hint");
}

static Object HHVM_METHOD(MongoCursor, immortal, bool liveForever) {
  throw_not_implemented("MongoCursor::immortal");
}

static Array HHVM_METHOD(MongoCursor, info) {
  throw_not_implemented("MongoCursor::info");
}

static String HHVM_METHOD(MongoCursor, key) {
  throw_not_implemented("MongoCursor::key");
}

static Object HHVM_METHOD(MongoCursor, limit, int64_t num) {
  throw_not_implemented("MongoCursor::limit");
}

static Object HHVM_METHOD(MongoCursor, maxTimeMS, int64_t ms) {
  throw_not_implemented("MongoCursor::maxTimeMS");
}

static void HHVM_METHOD(MongoCursor, next) {
  throw_not_implemented("MongoCursor::next");
}

static Object HHVM_METHOD(MongoCursor, partial, bool okay) {
  throw_not_implemented("MongoCursor::partial");
}

static void HHVM_METHOD(MongoCursor, reset) {
  throw_not_implemented("MongoCursor::reset");
}

static void HHVM_METHOD(MongoCursor, rewind) {
  throw_not_implemented("MongoCursor::rewind");
}

static Object HHVM_METHOD(MongoCursor, setFlag, int64_t flag, bool set) {
  throw_not_implemented("MongoCursor::setFlag");
}

static Object HHVM_METHOD(MongoCursor, setReadPreference, const String& read_preference, const Array& tags) {
  throw_not_implemented("MongoCursor::setReadPreference");
}

static Object HHVM_METHOD(MongoCursor, skip, int64_t num) {
  throw_not_implemented("MongoCursor::skip");
}

static Object HHVM_METHOD(MongoCursor, slaveOkay, bool okay) {
  throw_not_implemented("MongoCursor::slaveOkay");
}

static Object HHVM_METHOD(MongoCursor, snapshot) {
  throw_not_implemented("MongoCursor::snapshot");
}

static Object HHVM_METHOD(MongoCursor, sort, const Array& fields) {
  throw_not_implemented("MongoCursor::sort");
}

static Object HHVM_METHOD(MongoCursor, tailable, bool tail) {
  throw_not_implemented("MongoCursor::tailable");
}

static Object HHVM_METHOD(MongoCursor, timeout, int64_t ms) {
  throw_not_implemented("MongoCursor::timeout");
}

static bool HHVM_METHOD(MongoCursor, valid) {
  throw_not_implemented("MongoCursor::valid");
}

const StaticString s_MongoCursorException("MongoCursorException");
//////////////////////////////////////////////////////////////////////////////
// class MongoCursorException

static String HHVM_METHOD(MongoCursorException, getHost) {
  throw_not_implemented("MongoCursorException::getHost");
}

const StaticString s_MongoCursorTimeoutException("MongoCursorTimeoutException");
const StaticString s_MongoDate("MongoDate");
//////////////////////////////////////////////////////////////////////////////
// class MongoDate

static void HHVM_METHOD(MongoDate, __construct, int64_t sec, int64_t usec) {
  throw_not_implemented("MongoDate::__construct");
}

static String HHVM_METHOD(MongoDate, __toString) {
  throw_not_implemented("MongoDate::__toString");
}

const StaticString s_MongoDB("MongoDB");
//////////////////////////////////////////////////////////////////////////////
// class MongoDB

static Array HHVM_METHOD(MongoDB, authenticate, const String& username, const String& password) {
  throw_not_implemented("MongoDB::authenticate");
}

static Array HHVM_METHOD(MongoDB, command, const Array& command, const Array& options) {
  throw_not_implemented("MongoDB::command");
}

static void HHVM_METHOD(MongoDB, __construct, const Object& conn, const String& name) {
  throw_not_implemented("MongoDB::__construct");
}

static Object HHVM_METHOD(MongoDB, createCollection, const String& name, const Array& options) {
  throw_not_implemented("MongoDB::createCollection");
}

static Array HHVM_METHOD(MongoDB, createDBRef, const String& collection, const Variant& document_or_id) {
  throw_not_implemented("MongoDB::createDBRef");
}

static Array HHVM_METHOD(MongoDB, drop) {
  throw_not_implemented("MongoDB::drop");
}

static Array HHVM_METHOD(MongoDB, dropCollection, const Variant& coll) {
  throw_not_implemented("MongoDB::dropCollection");
}

static Array HHVM_METHOD(MongoDB, execute, const Variant& code, const Array& args) {
  throw_not_implemented("MongoDB::execute");
}

static bool HHVM_METHOD(MongoDB, forceError) {
  throw_not_implemented("MongoDB::forceError");
}

static Object HHVM_METHOD(MongoDB, __get, const String& name) {
  throw_not_implemented("MongoDB::__get");
}

static Array HHVM_METHOD(MongoDB, getCollectionNames, bool includeSystemCollections) {
  throw_not_implemented("MongoDB::getCollectionNames");
}

static Array HHVM_METHOD(MongoDB, getDBRef, const Array& ref) {
  throw_not_implemented("MongoDB::getDBRef");
}

static Object HHVM_METHOD(MongoDB, getGridFS, const String& prefix) {
  throw_not_implemented("MongoDB::getGridFS");
}

static int64_t HHVM_METHOD(MongoDB, getProfilingLevel) {
  throw_not_implemented("MongoDB::getProfilingLevel");
}

static Array HHVM_METHOD(MongoDB, getReadPreference) {
  throw_not_implemented("MongoDB::getReadPreference");
}

static bool HHVM_METHOD(MongoDB, getSlaveOkay) {
  throw_not_implemented("MongoDB::getSlaveOkay");
}

static Array HHVM_METHOD(MongoDB, getWriteConcern) {
  throw_not_implemented("MongoDB::getWriteConcern");
}

static Array HHVM_METHOD(MongoDB, lastError) {
  throw_not_implemented("MongoDB::lastError");
}

static Array HHVM_METHOD(MongoDB, listCollections, bool includeSystemCollections) {
  throw_not_implemented("MongoDB::listCollections");
}

static Array HHVM_METHOD(MongoDB, prevError) {
  throw_not_implemented("MongoDB::prevError");
}

static Array HHVM_METHOD(MongoDB, repair, bool preserve_cloned_files, bool backup_original_files) {
  throw_not_implemented("MongoDB::repair");
}

static Array HHVM_METHOD(MongoDB, resetError) {
  throw_not_implemented("MongoDB::resetError");
}

static Object HHVM_METHOD(MongoDB, selectCollection, const String& name) {
  throw_not_implemented("MongoDB::selectCollection");
}

static int64_t HHVM_METHOD(MongoDB, setProfilingLevel, int64_t level) {
  throw_not_implemented("MongoDB::setProfilingLevel");
}

static bool HHVM_METHOD(MongoDB, setReadPreference, const String& read_preference, const Array& tags) {
  throw_not_implemented("MongoDB::setReadPreference");
}

static bool HHVM_METHOD(MongoDB, setSlaveOkay, bool ok) {
  throw_not_implemented("MongoDB::setSlaveOkay");
}

static bool HHVM_METHOD(MongoDB, setWriteConcern, const Variant& w, int64_t wtimeout) {
  throw_not_implemented("MongoDB::setWriteConcern");
}

static String HHVM_METHOD(MongoDB, __toString) {
  throw_not_implemented("MongoDB::__toString");
}

const StaticString s_MongoDBRef("MongoDBRef");
//////////////////////////////////////////////////////////////////////////////
// class MongoDBRef

static Array HHVM_STATIC_METHOD(MongoDBRef, create, const String& collection, const Variant& id, const String& database) {
  throw_not_implemented("MongoDBRef::create");
}

static Array HHVM_STATIC_METHOD(MongoDBRef, get, const Object& db, const Array& ref) {
  throw_not_implemented("MongoDBRef::get");
}

static bool HHVM_STATIC_METHOD(MongoDBRef, isRef, const Variant& ref) {
  throw_not_implemented("MongoDBRef::isRef");
}

const StaticString s_MongoDeleteBatch("MongoDeleteBatch");
//////////////////////////////////////////////////////////////////////////////
// class MongoDeleteBatch

static void HHVM_METHOD(MongoDeleteBatch, __construct, const Object& collection, const Array& write_options) {
  throw_not_implemented("MongoDeleteBatch::__construct");
}

const StaticString s_MongoDuplicateKeyException("MongoDuplicateKeyException");
const StaticString s_MongoException("MongoException");
const StaticString s_MongoExecutionTimeoutException("MongoExecutionTimeoutException");
const StaticString s_MongoGridFS("MongoGridFS");
//////////////////////////////////////////////////////////////////////////////
// class MongoGridFS

static void HHVM_METHOD(MongoGridFS, __construct, const Object& db, const String& prefix, const Variant& chunks) {
  throw_not_implemented("MongoGridFS::__construct");
}

static Object HHVM_METHOD(MongoGridFS, delete, const Variant& id) {
  throw_not_implemented("MongoGridFS::delete");
}

static Array HHVM_METHOD(MongoGridFS, drop) {
  throw_not_implemented("MongoGridFS::drop");
}

static Object HHVM_METHOD(MongoGridFS, find, const Array& query, const Array& fields) {
  throw_not_implemented("MongoGridFS::find");
}

static Object HHVM_METHOD(MongoGridFS, findOne, const Variant& query, const Variant& fields) {
  throw_not_implemented("MongoGridFS::findOne");
}

static Object HHVM_METHOD(MongoGridFS, get, const Variant& id) {
  throw_not_implemented("MongoGridFS::get");
}

static Variant HHVM_METHOD(MongoGridFS, put, const String& filename, const Array& metadata, const Array& options) {
  throw_not_implemented("MongoGridFS::put");
}

static Object HHVM_METHOD(MongoGridFS, remove, const Array& criteria, const Array& options) {
  throw_not_implemented("MongoGridFS::remove");
}

static Variant HHVM_METHOD(MongoGridFS, storeBytes, const String& bytes, const Array& metadata, const Array& options) {
  throw_not_implemented("MongoGridFS::storeBytes");
}

static Variant HHVM_METHOD(MongoGridFS, storeFile, const String& filename, const Array& metadata, const Array& options) {
  throw_not_implemented("MongoGridFS::storeFile");
}

static Variant HHVM_METHOD(MongoGridFS, storeUpload, const String& name, const Array& metadata) {
  throw_not_implemented("MongoGridFS::storeUpload");
}

const StaticString s_MongoGridFSCursor("MongoGridFSCursor");
//////////////////////////////////////////////////////////////////////////////
// class MongoGridFSCursor

static void HHVM_METHOD(MongoGridFSCursor, __construct, const Object& gridfs, const Resource& connection, const String& ns, const Array& query, const Array& fields) {
  throw_not_implemented("MongoGridFSCursor::__construct");
}

static Object HHVM_METHOD(MongoGridFSCursor, current) {
  throw_not_implemented("MongoGridFSCursor::current");
}

static Object HHVM_METHOD(MongoGridFSCursor, getNext) {
  throw_not_implemented("MongoGridFSCursor::getNext");
}

static String HHVM_METHOD(MongoGridFSCursor, key) {
  throw_not_implemented("MongoGridFSCursor::key");
}

const StaticString s_MongoGridFSException("MongoGridFSException");
const StaticString s_MongoGridFSFile("MongoGridFSFile");
//////////////////////////////////////////////////////////////////////////////
// class MongoGridFSFile

static void HHVM_METHOD(MongoGridfsFile, __construct, const Object& gridfs, const Array& file) {
  throw_not_implemented("MongoGridfsFile::__construct");
}

static String HHVM_METHOD(MongoGridFSFile, getBytes) {
  throw_not_implemented("MongoGridFSFile::getBytes");
}

static String HHVM_METHOD(MongoGridFSFile, getFilename) {
  throw_not_implemented("MongoGridFSFile::getFilename");
}

static Object HHVM_METHOD(MongoGridFSFile, getResource) {
  throw_not_implemented("MongoGridFSFile::getResource");
}

static int64_t HHVM_METHOD(MongoGridFSFile, getSize) {
  throw_not_implemented("MongoGridFSFile::getSize");
}

static int64_t HHVM_METHOD(MongoGridFSFile, write, const String& filename) {
  throw_not_implemented("MongoGridFSFile::write");
}

const StaticString s_MongoId("MongoId");
//////////////////////////////////////////////////////////////////////////////
// class MongoId

static void HHVM_METHOD(MongoId, __construct, const String& id) {
  throw_not_implemented("MongoId::__construct");
}

static String HHVM_STATIC_METHOD(MongoId, getHostname) {
  throw_not_implemented("MongoId::getHostname");
}

static int64_t HHVM_METHOD(MongoId, getInc) {
  throw_not_implemented("MongoId::getInc");
}

static int64_t HHVM_METHOD(MongoId, getPID) {
  throw_not_implemented("MongoId::getPID");
}

static int64_t HHVM_METHOD(MongoId, getTimestamp) {
  throw_not_implemented("MongoId::getTimestamp");
}

static bool HHVM_STATIC_METHOD(MongoId, isValid, const Variant& value) {
  throw_not_implemented("MongoId::isValid");
}

static Object HHVM_STATIC_METHOD(MongoId, __set_state, const Array& props) {
  throw_not_implemented("MongoId::__set_state");
}

static String HHVM_METHOD(MongoId, __toString) {
  throw_not_implemented("MongoId::__toString");
}

const StaticString s_MongoInsertBatch("MongoInsertBatch");
//////////////////////////////////////////////////////////////////////////////
// class MongoInsertBatch

static void HHVM_METHOD(MongoInsertBatch, __construct, const Object& collection, const Array& write_options) {
  throw_not_implemented("MongoInsertBatch::__construct");
}

const StaticString s_MongoInt32("MongoInt32");
//////////////////////////////////////////////////////////////////////////////
// class MongoInt32

static void HHVM_METHOD(MongoInt32, __construct, const String& value) {
  throw_not_implemented("MongoInt32::__construct");
}

static String HHVM_METHOD(MongoInt32, __toString) {
  throw_not_implemented("MongoInt32::__toString");
}

const StaticString s_MongoInt64("MongoInt64");
//////////////////////////////////////////////////////////////////////////////
// class MongoInt64

static void HHVM_METHOD(MongoInt64, __construct, const String& value) {
  throw_not_implemented("MongoInt64::__construct");
}

static String HHVM_METHOD(MongoInt64, __toString) {
  throw_not_implemented("MongoInt64::__toString");
}

const StaticString s_MongoLog("MongoLog");
//////////////////////////////////////////////////////////////////////////////
// class MongoLog

static Object HHVM_STATIC_METHOD(MongoLog, getCallback) {
  throw_not_implemented("MongoLog::getCallback");
}

static int64_t HHVM_STATIC_METHOD(MongoLog, getLevel) {
  throw_not_implemented("MongoLog::getLevel");
}

static int64_t HHVM_STATIC_METHOD(MongoLog, getModule) {
  throw_not_implemented("MongoLog::getModule");
}

static void HHVM_STATIC_METHOD(MongoLog, setCallback, const Object& log_function) {
  throw_not_implemented("MongoLog::setCallback");
}

static void HHVM_STATIC_METHOD(MongoLog, setLevel, int64_t level) {
  throw_not_implemented("MongoLog::setLevel");
}

static void HHVM_STATIC_METHOD(MongoLog, setModule, int64_t module) {
  throw_not_implemented("MongoLog::setModule");
}

const StaticString s_MongoMaxKey("MongoMaxKey");
const StaticString s_MongoMinKey("MongoMinKey");
const StaticString s_MongoPool("MongoPool");
//////////////////////////////////////////////////////////////////////////////
// class MongoPool

static int64_t HHVM_STATIC_METHOD(MongoPool, getSize) {
  throw_not_implemented("MongoPool::getSize");
}

static Array HHVM_METHOD(MongoPool, info) {
  throw_not_implemented("MongoPool::info");
}

static bool HHVM_STATIC_METHOD(MongoPool, setSize, int64_t size) {
  throw_not_implemented("MongoPool::setSize");
}

const StaticString s_MongoProtocolException("MongoProtocolException");
const StaticString s_MongoRegex("MongoRegex");
//////////////////////////////////////////////////////////////////////////////
// class MongoRegex

static void HHVM_METHOD(MongoRegex, __construct, const String& regex) {
  throw_not_implemented("MongoRegex::__construct");
}

static String HHVM_METHOD(MongoRegex, __toString) {
  throw_not_implemented("MongoRegex::__toString");
}

const StaticString s_MongoResultException("MongoResultException");
//////////////////////////////////////////////////////////////////////////////
// class MongoResultException

static Array HHVM_METHOD(MongoResultException, getDocument) {
  throw_not_implemented("MongoResultException::getDocument");
}

const StaticString s_MongoTimestamp("MongoTimestamp");
//////////////////////////////////////////////////////////////////////////////
// class MongoTimestamp

static void HHVM_METHOD(MongoTimestamp, __construct, int64_t sec, int64_t inc) {
  throw_not_implemented("MongoTimestamp::__construct");
}

static String HHVM_METHOD(MongoTimestamp, __toString) {
  throw_not_implemented("MongoTimestamp::__toString");
}

const StaticString s_MongoUpdateBatch("MongoUpdateBatch");
//////////////////////////////////////////////////////////////////////////////
// class MongoUpdateBatch

static void HHVM_METHOD(MongoUpdateBatch, __construct, const Object& collection, const Array& write_options) {
  throw_not_implemented("MongoUpdateBatch::__construct");
}

const StaticString s_MongoWriteBatch("MongoWriteBatch");
//////////////////////////////////////////////////////////////////////////////
// class MongoWriteBatch

static bool HHVM_METHOD(MongoWriteBatch, add, const Array& item) {
  throw_not_implemented("MongoWriteBatch::add");
}

static Array HHVM_METHOD(MongoWriteBatch, execute, const Array& write_options) {
  throw_not_implemented("MongoWriteBatch::execute");
}

const StaticString s_MongoWriteConcernException("MongoWriteConcernException");
//////////////////////////////////////////////////////////////////////////////
// class MongoWriteConcernException

static Array HHVM_METHOD(MongoWriteConcernException, getDocument) {
  throw_not_implemented("MongoWriteConcernException::getDocument");
}

//////////////////////////////////////////////////////////////////////////////
// functions

static void HHVM_FUNCTION(log_cmd_delete, const Array& server, const Array& writeOptions, const Array& deleteOptions, const Array& protocolOptions) {
  throw_not_implemented("log_cmd_delete");
}

static void HHVM_FUNCTION(log_cmd_insert, const Array& server, const Array& document, const Array& writeOptions, const Array& protocolOptions) {
  throw_not_implemented("log_cmd_insert");
}

static void HHVM_FUNCTION(log_cmd_update, const Array& server, const Array& writeOptions, const Array& updateOptions, const Array& protocolOptions) {
  throw_not_implemented("log_cmd_update");
}

static void HHVM_FUNCTION(log_getmore, const Array& server, const Array& info) {
  throw_not_implemented("log_getmore");
}

static void HHVM_FUNCTION(log_killcursor, const Array& server, const Array& info) {
  throw_not_implemented("log_killcursor");
}

static void HHVM_FUNCTION(log_reply, const Array& server, const Array& messageHeaders, const Array& operationHeaders) {
  throw_not_implemented("log_reply");
}

static void HHVM_FUNCTION(log_write_batch, const Array& server, const Array& writeOptions, const Array& batch, const Array& protocolOptions) {
  throw_not_implemented("log_write_batch");
}

static Array HHVM_FUNCTION(bson_decode, const String& bson) {
  throw_not_implemented("bson_decode");
}

static String HHVM_FUNCTION(bson_encode, const Variant& anything) 
{
  throw_not_implemented("bson_encode");
}

void mongoExtension::moduleInit() 
{
    default_host_ = "localhost";
    default_port_ = 27017;
    
    manager_ = mongo_init();
    //TSRMLS_SET_CTX(mongo_globals->manager->log_context);
    manager_->log_function = php_mcon_log_wrapper;

    manager_->connect               = php_mongo_io_stream_connect;
    manager_->recv_header           = php_mongo_io_stream_read;
    manager_->recv_data             = php_mongo_io_stream_read;
    manager_->send                  = php_mongo_io_stream_send;
    manager_->close                 = php_mongo_io_stream_close;
    manager_->forget                = php_mongo_io_stream_forget;
    manager_->authenticate          = php_mongo_io_stream_authenticate;
    //manager_->supports_wire_version = php_mongo_api_supports_wire_version;

    HHVM_ME(Mongo, connectUtil);
    HHVM_STATIC_ME(Mongo, getPoolSize);
    HHVM_ME(Mongo, getSlave);
    HHVM_ME(Mongo, getSlaveOkay);
    HHVM_ME(Mongo, poolDebug);
    HHVM_STATIC_ME(Mongo, setPoolSize);
    HHVM_ME(Mongo, setSlaveOkay);
    HHVM_ME(Mongo, switchSlave);
    HHVM_ME(MongoBinData, __construct);
    HHVM_ME(MongoBinData, __toString);

    HHVM_ME(MongoClient, __construct);
    HHVM_ME(MongoClient, close);
    HHVM_ME(MongoClient, connect);
    HHVM_ME(MongoClient, dropDB);
    HHVM_ME(MongoClient, __get);
    HHVM_STATIC_ME(MongoClient, getConnections);
    HHVM_ME(MongoClient, getHosts);
    HHVM_ME(MongoClient, getReadPreference);
    HHVM_ME(MongoClient, getWriteConcern);
    HHVM_ME(MongoClient, killCursor);
    HHVM_ME(MongoClient, listDBs);
    HHVM_ME(MongoClient, selectCollection);
    HHVM_ME(MongoClient, selectDB);
    HHVM_ME(MongoClient, setReadPreference);
    HHVM_ME(MongoClient, setWriteConcern);
    HHVM_ME(MongoClient, __toString);

    HHVM_ME(MongoCode, __construct);
    HHVM_ME(MongoCode, __toString);

    HHVM_ME(MongoCollection, aggregate);
    HHVM_ME(MongoCollection, aggregateCursor);
    HHVM_ME(MongoCollection, batchInsert);
    HHVM_ME(MongoCollection, __construct);
    HHVM_ME(MongoCollection, count);
    HHVM_ME(MongoCollection, createDBRef);
    HHVM_ME(MongoCollection, createIndex);
    HHVM_ME(MongoCollection, deleteIndex);
    HHVM_ME(MongoCollection, deleteIndexes);
    HHVM_ME(MongoCollection, distinct);
    HHVM_ME(MongoCollection, drop);
    HHVM_ME(MongoCollection, ensureIndex);
    HHVM_ME(MongoCollection, find);
    HHVM_ME(MongoCollection, findAndModify);
    HHVM_ME(MongoCollection, findOne);
    HHVM_ME(MongoCollection, __get);
    HHVM_ME(MongoCollection, getDBRef);
    HHVM_ME(MongoCollection, getIndexInfo);
    HHVM_ME(MongoCollection, getName);
    HHVM_ME(MongoCollection, getReadPreference);
    HHVM_ME(MongoCollection, getSlaveOkay);
    HHVM_ME(MongoCollection, getWriteConcern);
    HHVM_ME(MongoCollection, group);
    HHVM_ME(MongoCollection, insert);
    HHVM_ME(MongoCollection, parallelCollectionScan);
    HHVM_ME(MongoCollection, remove);
    HHVM_ME(MongoCollection, save);
    HHVM_ME(MongoCollection, setReadPreference);
    HHVM_ME(MongoCollection, setSlaveOkay);
    HHVM_ME(MongoCollection, setWriteConcern);
    HHVM_ME(MongoCollection, toIndexString);
    HHVM_ME(MongoCollection, __toString);
    HHVM_ME(MongoCollection, update);
    HHVM_ME(MongoCollection, validate);

    HHVM_ME(MongoCommandCursor, batchSize);
    HHVM_ME(MongoCommandCursor, __construct);
    HHVM_ME(MongoCommandCursor, createFromDocument);
    HHVM_ME(MongoCommandCursor, current);
    HHVM_ME(MongoCommandCursor, dead);
    HHVM_ME(MongoCommandCursor, info);
    HHVM_ME(MongoCommandCursor, key);
    HHVM_ME(MongoCommandCursor, next);
    HHVM_ME(MongoCommandCursor, rewind);
    HHVM_ME(MongoCommandCursor, valid);

    HHVM_ME(MongoCursor, addOption);
    HHVM_ME(MongoCursor, awaitData);
    HHVM_ME(MongoCursor, batchSize);
    HHVM_ME(MongoCursor, __construct);
    HHVM_ME(MongoCursor, count);
    HHVM_ME(MongoCursor, current);
    HHVM_ME(MongoCursor, dead);
    HHVM_ME(MongoCursor, doQuery);
    HHVM_ME(MongoCursor, explain);
    HHVM_ME(MongoCursor, fields);
    HHVM_ME(MongoCursor, getNext);
    HHVM_ME(MongoCursor, getReadPreference);
    HHVM_ME(MongoCursor, hasNext);
    HHVM_ME(MongoCursor, hint);
    HHVM_ME(MongoCursor, immortal);
    HHVM_ME(MongoCursor, info);
    HHVM_ME(MongoCursor, key);
    HHVM_ME(MongoCursor, limit);
    HHVM_ME(MongoCursor, maxTimeMS);
    HHVM_ME(MongoCursor, next);
    HHVM_ME(MongoCursor, partial);
    HHVM_ME(MongoCursor, reset);
    HHVM_ME(MongoCursor, rewind);
    HHVM_ME(MongoCursor, setFlag);
    HHVM_ME(MongoCursor, setReadPreference);
    HHVM_ME(MongoCursor, skip);
    HHVM_ME(MongoCursor, slaveOkay);
    HHVM_ME(MongoCursor, snapshot);
    HHVM_ME(MongoCursor, sort);
    HHVM_ME(MongoCursor, tailable);
    HHVM_ME(MongoCursor, timeout);
    HHVM_ME(MongoCursor, valid);

    HHVM_ME(MongoCursorException, getHost);

    HHVM_ME(MongoDate, __construct);
    HHVM_ME(MongoDate, __toString);

    HHVM_ME(MongoDB, authenticate);
    HHVM_ME(MongoDB, command);
    HHVM_ME(MongoDB, __construct);
    HHVM_ME(MongoDB, createCollection);
    HHVM_ME(MongoDB, createDBRef);
    HHVM_ME(MongoDB, drop);
    HHVM_ME(MongoDB, dropCollection);
    HHVM_ME(MongoDB, execute);
    HHVM_ME(MongoDB, forceError);
    HHVM_ME(MongoDB, __get);
    HHVM_ME(MongoDB, getCollectionNames);
    HHVM_ME(MongoDB, getDBRef);
    HHVM_ME(MongoDB, getGridFS);
    HHVM_ME(MongoDB, getProfilingLevel);
    HHVM_ME(MongoDB, getReadPreference);
    HHVM_ME(MongoDB, getSlaveOkay);
    HHVM_ME(MongoDB, getWriteConcern);
    HHVM_ME(MongoDB, lastError);
    HHVM_ME(MongoDB, listCollections);
    HHVM_ME(MongoDB, prevError);
    HHVM_ME(MongoDB, repair);
    HHVM_ME(MongoDB, resetError);
    HHVM_ME(MongoDB, selectCollection);
    HHVM_ME(MongoDB, setProfilingLevel);
    HHVM_ME(MongoDB, setReadPreference);
    HHVM_ME(MongoDB, setSlaveOkay);
    HHVM_ME(MongoDB, setWriteConcern);
    HHVM_ME(MongoDB, __toString);

    HHVM_STATIC_ME(MongoDBRef, create);
    HHVM_STATIC_ME(MongoDBRef, get);
    HHVM_STATIC_ME(MongoDBRef, isRef);

    HHVM_ME(MongoDeleteBatch, __construct);
    HHVM_ME(MongoGridFS, __construct);
    HHVM_ME(MongoGridFS, delete);
    HHVM_ME(MongoGridFS, drop);
    HHVM_ME(MongoGridFS, find);
    HHVM_ME(MongoGridFS, findOne);
    HHVM_ME(MongoGridFS, get);
    HHVM_ME(MongoGridFS, put);
    HHVM_ME(MongoGridFS, remove);
    HHVM_ME(MongoGridFS, storeBytes);
    HHVM_ME(MongoGridFS, storeFile);
    HHVM_ME(MongoGridFS, storeUpload);
    HHVM_ME(MongoGridFSCursor, __construct);
    HHVM_ME(MongoGridFSCursor, current);
    HHVM_ME(MongoGridFSCursor, getNext);
    HHVM_ME(MongoGridFSCursor, key);
    HHVM_ME(MongoGridfsFile, __construct);
    HHVM_ME(MongoGridFSFile, getBytes);
    HHVM_ME(MongoGridFSFile, getFilename);
    HHVM_ME(MongoGridFSFile, getResource);
    HHVM_ME(MongoGridFSFile, getSize);
    HHVM_ME(MongoGridFSFile, write);
    HHVM_ME(MongoId, __construct);
    HHVM_STATIC_ME(MongoId, getHostname);
    HHVM_ME(MongoId, getInc);
    HHVM_ME(MongoId, getPID);
    HHVM_ME(MongoId, getTimestamp);
    HHVM_STATIC_ME(MongoId, isValid);
    HHVM_STATIC_ME(MongoId, __set_state);
    HHVM_ME(MongoId, __toString);
    HHVM_ME(MongoInsertBatch, __construct);
    HHVM_ME(MongoInt32, __construct);
    HHVM_ME(MongoInt32, __toString);
    HHVM_ME(MongoInt64, __construct);
    HHVM_ME(MongoInt64, __toString);
    HHVM_STATIC_ME(MongoLog, getCallback);
    HHVM_STATIC_ME(MongoLog, getLevel);
    HHVM_STATIC_ME(MongoLog, getModule);
    HHVM_STATIC_ME(MongoLog, setCallback);
    HHVM_STATIC_ME(MongoLog, setLevel);
    HHVM_STATIC_ME(MongoLog, setModule);
    HHVM_STATIC_ME(MongoPool, getSize);
    HHVM_ME(MongoPool, info);
    HHVM_STATIC_ME(MongoPool, setSize);
    HHVM_ME(MongoRegex, __construct);
    HHVM_ME(MongoRegex, __toString);
    HHVM_ME(MongoResultException, getDocument);
    HHVM_ME(MongoTimestamp, __construct);
    HHVM_ME(MongoTimestamp, __toString);
    HHVM_ME(MongoUpdateBatch, __construct);
    HHVM_ME(MongoWriteBatch, add);
    HHVM_ME(MongoWriteBatch, execute);
    HHVM_ME(MongoWriteConcernException, getDocument);
    HHVM_FE(log_cmd_delete);
    HHVM_FE(log_cmd_insert);
    HHVM_FE(log_cmd_update);
    HHVM_FE(log_getmore);
    HHVM_FE(log_killcursor);
    HHVM_FE(log_reply);
    HHVM_FE(log_write_batch);
    HHVM_FE(bson_decode);
    HHVM_FE(bson_encode);
    loadSystemlib();
}

mongoExtension s_mongo_extension;

HHVM_GET_MODULE(mongo);
} // namespace HPHP
