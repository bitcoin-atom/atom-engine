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

    int port = 0;
    for (int i = 0; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find(("port=") == 0)) {
            std::string portStr = arg.substr(5);
            std::stringstream ss;
            ss << portStr;
            ss >> port;
        }
    }

    AtomEngineServer atomEngineServer(port);
    if (atomEngineServer.run()) {
        return a.exec();
    } else {
        return 0;
    }
}
