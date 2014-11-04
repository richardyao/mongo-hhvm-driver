// Copyright (c) 2014. All rights reserved.
// ref: https://github.com/google/lmctfy/blob/master/base/stringprintf.cc

#include "stringprintf.h"

#include <errno.h>
#include <stdarg.h> // For va_list and related operations
#include <stdio.h>  // MSVC requires this for _vsnprintf
#include <vector>

using std::vector;

void StringAppendV(string* dst, const char* format, va_list ap)
{
    // First try with a small fixed size buffer
    static const int kSpaceLength = 1024;
    char space[kSpaceLength];

    // It's possible for methods that use a va_list to invalidate
    // th data in it upon use. The fix is to make a copy
    // of the structure before using it and use that copy instead.
    va_list backup_ap;
    va_copy(backup_ap, ap);
    int result = vsnprintf(space, kSpaceLength, format, backup_ap);
    va_end(backup_ap);

    if (result < kSpaceLength) {
        if (result >= 0) {
            // Normal case -- everything fit.
            dst->append(space, result);
            return;
        }

        if (result < 0) {
            // Just an error.
            return;
        }
    }

    // Increase the buffer size to the size requested by vsnprintf,
    // plus one for the closing \0.
    int length = result + 1;
    char* buf = new char[length];

    // Restore the va_list before we use it again
    va_copy(backup_ap, ap);
    result = vsnprintf(buf, length, format, backup_ap);
    va_end(backup_ap);

    if (result >= 0 && result < length) {
        // It fit
        dst->append(buf, result);
    }
    delete [] buf;
}

string StringPrintf(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    string result;
    StringAppendV(&result, format, ap);
    va_end(ap);
    return result;
}

const string& SStringPrintf(string* dst, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    dst->clear();
    StringAppendV(dst, format, ap);
    va_end(ap);
    return *dst;
}

void StringAppendF(string* dst, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    StringAppendV(dst, format, ap);
    va_end(ap);
}
