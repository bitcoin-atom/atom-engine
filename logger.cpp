// Copyright 2018 AtomicSwap Solutions Ltd. All rights reserved.
// Use of this source code is governed by Microsoft Reference Source
// License (MS-RSL) that can be found in the LICENSE file.

#include "logger.h"
#include <QDebug>
#include <QTextStream>
#include <QDateTime>
#include <QDir>

namespace {
    const QString loggerFileName = "info.log";
}

Logger::Logger() :
    file(nullptr)
{
    file = new QFile;
    file->setFileName(loggerFileName);
    file->open(QIODevice::WriteOnly | QIODevice::Text);
}

Logger::~Logger()
{
    if (file) {
        file->close();
        delete file;
    }
}

Logger& Logger::info()
{
    static Logger logger;
    return logger;
}

void Logger::operator << (const QString& str)
{
    int pos = str.lastIndexOf("\n");
    qDebug().noquote() << str.left(pos);
    if (file) {
        QTextStream out(file);
        out << "[" << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << "] " << str.left(pos) << endl;
    }
}
