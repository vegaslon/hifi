//
//  CrashHandler_None.cpp
//  interface/src
//
//  Created by Clement Brisset on 01/19/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#if !defined(HAS_CRASHPAD) && !defined(HAS_BREAKPAD)

#include "CrashHandler.h"

#include <assert.h>

#include <QDebug>

bool startCrashHandler(std::string appPath) {
    qDebug() << "No crash handler available.";
    return false;
}

void setCrashAnnotation(std::string name, std::string value) {
}

#endif
