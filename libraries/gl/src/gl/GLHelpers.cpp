#include "GLHelpers.h"

#include <mutex>

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QRegularExpression>
#include <QtCore/QProcessEnvironment>

#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLDebugLogger>

#include <QtOpenGL/QGL>

size_t evalGLFormatSwapchainPixelSize(const QSurfaceFormat& format) {
    size_t pixelSize = format.redBufferSize() + format.greenBufferSize() + format.blueBufferSize() + format.alphaBufferSize();
    // We don't apply the length of the swap chain into this pixelSize since it is not vsible for the Process (on windows).
    // Let s keep this here remember that:
    // if (format.swapBehavior() > 0) {
    //     pixelSize *= format.swapBehavior(); // multiply the color buffer pixel size by the actual swapchain depth
    // }
    pixelSize += format.stencilBufferSize() + format.depthBufferSize();
    return pixelSize;
}

const QSurfaceFormat& getDefaultOpenGLSurfaceFormat() {
    static QSurfaceFormat format;
    static std::once_flag once;
    std::call_once(once, [] {
#if defined(QT_OPENGL_ES_3_1)
        format.setRenderableType(QSurfaceFormat::OpenGLES);
        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
        format.setAlphaBufferSize(8);
#endif
        // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
        format.setDepthBufferSize(DEFAULT_GL_DEPTH_BUFFER_BITS);
        format.setStencilBufferSize(DEFAULT_GL_STENCIL_BUFFER_BITS);
        setGLFormatVersion(format);
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
        QSurfaceFormat::setDefaultFormat(format);
    });
    return format;
}

int glVersionToInteger(QString glVersion) {
    QStringList versionParts = glVersion.split(QRegularExpression("[\\.\\s]"));
    int majorNumber = 0, minorNumber = 0;
    if (versionParts.size() >= 2) {
        majorNumber = versionParts[0].toInt();
        minorNumber = versionParts[1].toInt();
    }
    return (majorNumber << 16) | minorNumber;
}

QJsonObject getGLContextData() {
    static QJsonObject result;
    static std::once_flag once;
    std::call_once(once, [] {
        QString glVersion = QString((const char*)glGetString(GL_VERSION));
        QString glslVersion = QString((const char*) glGetString(GL_SHADING_LANGUAGE_VERSION));
        QString glVendor = QString((const char*) glGetString(GL_VENDOR));
        QString glRenderer = QString((const char*)glGetString(GL_RENDERER));

        result = QJsonObject {
            { "version", glVersion },
            { "sl_version", glslVersion },
            { "vendor", glVendor },
            { "renderer", glRenderer },
        };
    });
    return result;
}

QThread* RENDER_THREAD = nullptr;

bool isRenderThread() {
    return QThread::currentThread() == RENDER_THREAD;
}

namespace gl {
    void withSavedContext(const std::function<void()>& f) {
        // Save the original GL context, because creating a QML surface will create a new context
        QOpenGLContext * savedContext = QOpenGLContext::currentContext();
        QSurface * savedSurface = savedContext ? savedContext->surface() : nullptr;
        f();
        if (savedContext) {
            savedContext->makeCurrent(savedSurface);
        }
    }
}
