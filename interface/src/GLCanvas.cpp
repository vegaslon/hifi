//
//  GLCanvas.cpp
//  interface/src
//
//  Created by Stephen Birarda on 8/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLCanvas.h"

#include "Application.h"

bool GLCanvas::event(QEvent* event) {
    if (QEvent::Paint == event->type() && qApp->isAboutToQuit()) {
        return true;
    }
    return GLWidget::event(event);
}
