// Copyright (c) 2014. All rights reserved.

#ifndef EXT_MONGO_H
#define EXT_MONGO_H

#include "hphp/runtime/base/base-includes.h"
#include "mcon/manager.h"

namespace HPHP {

class mongoExtension : public Extension {
public:
    mongoExtension() : Extension("mongo"), manager_(nullptr) {}
    virtual void moduleInit();

public:
    /* php.ini options */
    char* default_host_;
    long default_port_;

    /* _id generation helpers */
    int inc, pid, machine;

    /* timestamp generation helper */
    long ts_inc;
    char *errmsg;

    long log_level;
    long log_module;
    //zend_fcall_info log_callback_info;
    //zend_fcall_info_cache log_callback_info_cache;

    long ping_interval;
    long ismaster_interval;

    mongo_con_manager *manager_;
};

extern mongoExtension s_mongo_extension;
}

#endif // EXT_MONGO_H
