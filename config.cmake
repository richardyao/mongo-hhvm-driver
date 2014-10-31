HHVM_EXTENSION(mongo src/ext_mongo.cpp src/mcon/parse.c src/mcon/bson_helpers.c src/mcon/manager.c src/mcon/read_preference.c src/mcon/collection.c src/mcon/mini_bson.c src/mcon/str.c src/mcon/connections.c src/mcon/parse.c src/mcon/utils.c src/mcon/contrib/md5.c src/mcon/contrib/strndup.c)
HHVM_SYSTEMLIB(mongo src/ext_mongo.php)
