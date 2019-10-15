// Copyright 2018 AtomicSwap Solutions Ltd. All rights reserved.
// Use of this source code is governed by Microsoft Reference Source
// License (MS-RSL) that can be found in the LICENSE file.

#include <QCoreApplication>
#include "atomengineserver.h"
#include <string>
#include <sstream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    AtomEngineServer atomEngineServer;
    if (atomEngineServer.run()) {
        return a.exec();
    } else {
        return 0;
    }
}
