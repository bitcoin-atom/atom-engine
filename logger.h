// Copyright 2018 AtomicSwap Solutions Ltd. All rights reserved.
// Use of this source code is governed by Microsoft Reference Source
// License (MS-RSL) that can be found in the LICENSE file.

#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>

class Logger
{
public:
    static Logger& info();
    void operator << (const QString& str);
private:
    Logger();
    ~Logger();
private:
    QFile* file;
};

#endif // LOGGER_H
