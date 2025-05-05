QT += quick

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        diskmanager.cpp \
        include/dog/dog.cpp \
        include/dog/dogbase.cpp \
        include/dog/dogdiag.cpp \
        include/dog/dogenc64.cpp \
        include/dog/dogfeature.cpp \
        include/dog/dogfile.cpp \
        include/dog/doghandle.cpp \
        include/dog/dogimpl.cpp \
        include/dog/doginfo.cpp \
        include/dog/doglock.cpp \
        include/dog/dogmap.cpp \
        include/dog/dogtime.cpp \
        include/dog/dogversion.cpp \
        include/dog/errorprinter.cpp \
        main.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

INCLUDEPATH += $$PWD/include/dog
INCLUDEPATH += $$PWD/lib/dog_lib/x64

LIBS       += -L$$PWD/lib/dog_lib/x64   \
              -ldog_windows_x64_3163570

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    diskmanager.h \
    include/dog/dog_api.h \
    include/dog/dog_api_cpp.h \
    include/dog/dog_api_cpp_.h \
    include/dog/dogdiag.h \
    include/dog/errorprinter.h \
    include/dog/resource.h \
    include/dog/vendor_code.h

QMAKE_CXXFLAGS += /source-charset:utf-8
