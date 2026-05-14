QT += core gui serialport widgets

TARGET = QRadar
TEMPLATE = app

# 使用 C++11 标准
CONFIG += c++11

# 源文件
SOURCES += \
        main.cpp \
        widget.cpp

# 头文件
HEADERS += \
        widget.h

# 界面文件
FORMS += \
        widget.ui

# 默认部署路径设置
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
