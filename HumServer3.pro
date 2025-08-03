QT += core network websockets webchannel httpserver serialport sql

CONFIG += c++17
CONFIG += embed_translations
CONFIG -= app_bundle

TRANSLATIONS += \
    HumServer3_en_US.ts

TEMPLATE = app

SOURCES += \
    MariaDBInterface.cpp \
    controllerinterface.cpp \
    databridge.cpp \
    licenseserverinterface.cpp \
    main.cpp \
    settings.cpp \
    systemkeystore.cpp


HEADERS += \
    MariaDBInterface.h \
    controllerinterface.h \
    databridge.h \
    humatric_protocol.h \
    humtoken.h \
    licenseserverinterface.h \
    settings.h \
    systemkeystore.h \
    websockettransport.h

RESOURCES +=

win32 {
    # Per Windows: Esegui tutti i comandi di copia in una singola riga 'cmd /c'
    # separando i comandi con '&&' per assicurarne l'esecuzione sequenziale.
    # Le virgolette attorno ai percorsi sono essenziali per gestire spazi.
    QMAKE_POST_LINK += cmd /c copy /Y \"$$PWD\\index.html\" \"$$OUT_PWD\\index.html\" && \
                    copy /Y \"$$PWD\\script.js\" \"$$OUT_PWD\\script.js\" && \
                    copy /Y \"$$PWD\\qwebchannel.js\" \"$$OUT_PWD\\qwebchannel.js\"
}
unix {
    # Per Linux e macOS: Esegui tutti i comandi di copia in una singola riga 'cp'
    # separando i comandi con ';' o '&&'
    QMAKE_POST_LINK += cp -f $$PWD/index.html $$OUT_PWD/index.html && \
                    cp -f $$PWD/script.js $$OUT_PWD/script.js && \
                    cp -f $$PWD/qwebchannel.js $$OUT_PWD/qwebchannel.js
}


DISTFILES += \
    index.html \
    qwebchannel.js \
    script.js



