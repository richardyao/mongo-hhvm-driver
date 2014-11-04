// Copyright (c) 2014. All rights reserved.

#ifndef EXT_MONGO_H
#define EXT_MONGO_H

#include "hphp/runtime/base/base-includes.h"

namespace HPHP {

class mongoExtension : public Extension {
public:
    mongoExtension() : Extension("mongo"), manager_(nullptr) {}
    virtual void moduleInit();

public:
    /* php.ini options */
    char* default_host_;
    long default_port_;
    
    mongo_con_manager *manager_;
};

extern mongoExtension s_mongo_extension;
}

#endif // EXT_MONGO_H
