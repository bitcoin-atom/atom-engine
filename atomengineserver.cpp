// Copyright 2018 AtomicSwap Solutions Ltd. All rights reserved.
// Use of this source code is governed by Microsoft Reference Source
// License (MS-RSL) that can be found in the LICENSE file.

#include "atomengineserver.h"
#include <QDebug>
#include <QJsonArray>
#include <QTextStream>
#include <QStringList>
#include "info.h"
#include <QByteArrayList>
#include "logger.h"

namespace {
    const QString backupFileName = "info.dat";
    const QString curVersion = "0.2";
}

AtomEngineServer::AtomEngineServer(int port) :
    curOrderId_(0),
    curTradeId_(0),
    backupFile_(backupFileName),
    port_(port)
{
    server_ = new QTcpServer();
    connect(server_, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}

AtomEngineServer::~AtomEngineServer()
{
    for (auto it = connections_.begin(); it != connections_.end(); ++it) {
        it->second->close();
    }
    delete server_;
    Logger::info() << "Atom engine was closed";
}

bool AtomEngineServer::run()
{
    Logger::info() << "Atom engine start";
    if (!port_) {
        Logger::info() << "Start failed: need set a port";
        return false;
    }
    load();
    if (server_->listen(QHostAddress::Any, port_)) {
        Logger::info() << "Atom engine was started success, port = " + QString::number(port_) + " version = " + curVersion;
        return true;
    } else {
        Logger::info() << "Atom engine starting failed";
        return false;
    }
}

void AtomEngineServer::onNewConnection()
{
    QTcpSocket* clientSocket = server_->nextPendingConnection();
    connect(clientSocket, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
    int socketId = clientSocket->socketDescriptor();
    connections_[socketId] = clientSocket;
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    Logger::info() << "New connection id = " + QString::number(socketId) + ", active connections = " + QString::number(connections_.size());
}

void AtomEngineServer::onClientDisconnected()
{
    QTcpSocket* clientSocket = (QTcpSocket*)sender();

    auto itConnection = connections_.begin();
    while (itConnection != connections_.end()) {
        if (itConnection->second == clientSocket) {
            auto itAddr = addrs_.begin();
            while (itAddr != addrs_.end()) {
                if (itAddr->second == itConnection->first) {
                    itAddr = addrs_.erase(itAddr);
                } else {
                    ++itAddr;
                }
            }
            auto itBuf = buffers_.find(itConnection->first);
            if (itBuf != buffers_.end()) {
                buffers_.erase(itBuf);
            }
            itConnection = connections_.erase(itConnection);
        } else {
            ++itConnection;
        }
    }

    Logger::info() << "Client disconnected, active connections = " + QString::number(connections_.size());
}

void AtomEngineServer::onReadyRead()
{
    QTcpSocket* clientSocket = (QTcpSocket*)sender();
    QByteArray& buffer = buffers_[clientSocket->socketDescriptor()];

    QByteArray answer = clientSocket->readAll();
    buffer.append(answer);

    int pos = buffer.lastIndexOf("\n");
    if (pos < 0) {
        return;
    }

    QByteArrayList commands = buffer.left(pos).split('\n');
    int rightCount = buffer.length() - pos - 1;
    if (rightCount > 0) {
        buffer = buffer.right(rightCount);
    } else {
        buffer.clear();
    }

    for (int i = 0; i < commands.size(); ++i) {
        QByteArray& commandStr = commands[i];
        if (commandStr.length() == 0) {
            continue;
        }
        QJsonDocument doc = QJsonDocument::fromJson(commandStr);
        commandStr.append("\n");
        Logger::info() << QString("client descr = ") + QString::number(clientSocket->socketDescriptor()) + QString(" ") + commandStr;
        if (doc.isObject()) {
            QJsonObject req = doc.object();
            QString command = req["command"].toString();
            if (command == "init") {
                Addrs activeAddrs;
                QJsonArray curs = req["curs"].toArray();
                for (int i = 0; i < curs.size(); ++i) {
                    QJsonObject curInfo = curs[i].toObject();
                    QJsonArray addrs = curInfo["addrs"].toArray();
                    for (int i = 0; i < addrs.size(); ++i) {
                        QString addr = addrs[i].toString();
                        addrs_[addr] = clientSocket->socketDescriptor();
                        activeAddrs.insert(addr);
                    }
                }
                QString rep = "{\"reply\": \"init_success\", \"isActual\": true, \"orders\": [";
                for (auto it = orders_.begin(); it != orders_.end(); ++it) {
                    if (it != orders_.begin()) {
                        rep += ", ";
                    }
                    rep += it->second->getJson();
                }
                rep += "], \"trades\": [";
                bool firstTrade = true;
                for (auto it = trades_.begin(); it != trades_.end(); ++it) {
                    const QString& addr1 = it->second->order_->getAddress_;
                    const QString& addr2 = it->second->initiatorAddress_;
                    if (activeAddrs.find(addr1) != activeAddrs.end() || activeAddrs.find(addr2) != activeAddrs.end()) {
                        if (!firstTrade) {
                            rep +=  ", ";
                        }
                        rep += it->second->getJson();
                        firstTrade = false;
                    }
                }
                rep += "], \"commissions\": []}\n";
                clientSocket->write(rep.toStdString().c_str());
            }
            if (command == "request_swap_commission") {
                QJsonArray curs = req["curs"].toArray();
                for (int i = 0; i < curs.size(); ++i) {
                    QJsonObject curInfo = curs[i].toObject();
                    QJsonArray addrs = curInfo["addrs"].toArray();
                    for (int i = 0; i < addrs.size(); ++i) {
                        QString addr = addrs[i].toString();
                        addrs_[addr] = clientSocket->socketDescriptor();
                    }
                }
                QString rep = "{\"reply\": \"request_swap_commission_success\", \"commissions\": []}\n";
                clientSocket->write(rep.toStdString().c_str());
            }
            if (command == "create_order") {
                QJsonObject orderJson = req["order"].toObject();
                QString key = "";
                if (req.contains("key")) {
                    key = req["key"].toString();
                }
                OrderInfoPtr newOrder = createOrder(key, orderJson);
                if (newOrder) {
                    saveCommand(doc);
                    QString rep1 = "{\"reply\": \"create_order_success\", \"order\": " + newOrder->getJson() + "}\n";
                    QString rep2 = "{\"reply\": \"create_order\", \"order\": " + newOrder->getJson() + "}\n";
                    clientSocket->write(rep1.toStdString().c_str());
                    for (auto it = connections_.begin(); it != connections_.end(); ++it) {
                        if (it->first != clientSocket->socketDescriptor()) {
                            it->second->write(rep2.toStdString().c_str());
                        }
                    }
                    addrs_[newOrder->getAddress_] = clientSocket->socketDescriptor();
                }
            }
            if (command == "delete_order") {
                long long id = req["id"].toVariant().toLongLong();
                QString key = "";
                if (req.contains("key")) {
                    key = req["key"].toString();
                }
                bool deleted = deleteOrder(key, id);
                QString rep1 = "{\"reply\": \"delete_order_success\", \"id\": " + QString::number(id) + "}\n";
                clientSocket->write(rep1.toStdString().c_str());
                if (deleted) {
                    saveCommand(doc);
                    QString rep2 = "{\"reply\": \"delete_order\", \"id\": " + QString::number(id) + "}\n";
                    for (auto it = connections_.begin(); it != connections_.end(); ++it) {
                        if (it->first != clientSocket->socketDescriptor()) {
                            it->second->write(rep2.toStdString().c_str());
                        }
                    }
                }
            }
            if (command == "create_trade") {
                long long orderId = req["orderId"].toVariant().toLongLong();
                QString initiatorAddr = req["address"].toString();
                addrs_[initiatorAddr] = clientSocket->socketDescriptor();
                QString key = "";
                if (req.contains("key")) {
                    key = req["key"].toString();
                }
                TradeInfoPtr trade = createTrade(key, orderId, initiatorAddr);
                if (trade) {
                    saveCommand(doc);

                    QString rep1 = "{\"reply\": \"delete_order\", \"id\": " + QString::number(orderId) + "}\n";
                    QString rep2 = "{\"reply\": \"create_trade\", \"trade\": " + trade->getJson() + "}\n";
                    QString rep3 = "{\"reply\": \"create_trade_success\", \"trade\": " + trade->getJson() + "}\n";

                    clientSocket->write(rep3.toStdString().c_str());

                    int firstSocketDescr = clientSocket->socketDescriptor();
                    int secondSocketDescr = -1;
                    auto it = addrs_.find(trade->order_->getAddress_);
                    if (it != addrs_.end()) {
                        auto itCon = connections_.find(it->second);
                        if (itCon != connections_.end()) {
                            secondSocketDescr = itCon->first;
                            if (secondSocketDescr != firstSocketDescr) {
                                itCon->second->write(rep2.toStdString().c_str());
                            }
                        }
                    }

                    for (auto it = connections_.begin(); it != connections_.end(); ++it) {
                        if (it->first != firstSocketDescr && it->first != secondSocketDescr) {
                            it->second->write(rep1.toStdString().c_str());
                        }
                    }
                } else {
                    QString rep = "{\"reply\": \"create_trade_failed\", \"reasone\": \"order out of date\"}\n";
                    clientSocket->write(rep.toStdString().c_str());
                }
            }
            if (command == "update_trade") {
                QJsonObject tradeJson = req["trade"].toObject();
                QString key = "";
                if (req.contains("key")) {
                    key = req["key"].toString();
                }
                TradeInfoPtr trade = updateTrade(key, tradeJson);
                QString rep1 = "{\"reply\": \"update_trade_success\"}\n";
                clientSocket->write(rep1.toStdString().c_str());
                if (trade) {
                    saveCommand(doc);
                    QString rep2 = "{\"reply\": \"update_trade\", \"trade\": " + trade->getJson() + "}\n";
                    const QString& firstAddr = trade->order_->getAddress_;
                    const QString& secondAddr = trade->initiatorAddress_;
                    auto itFirstDescr = addrs_.find(firstAddr);
                    auto itSecondDescr = addrs_.find(secondAddr);
                    int anotherConnectionDescr = -1;
                    if (itFirstDescr != addrs_.end() && itSecondDescr != addrs_.end() && itFirstDescr->second == clientSocket->socketDescriptor()) {
                        anotherConnectionDescr = itSecondDescr->second;
                    } else if (itFirstDescr != addrs_.end() && itSecondDescr != addrs_.end()) {
                        anotherConnectionDescr = itFirstDescr->second;
                    }
                    if (anotherConnectionDescr != -1) {
                        auto it = connections_.find(anotherConnectionDescr);
                        if (it != connections_.end()) {
                            it->second->write(rep2.toStdString().c_str());
                        }
                    }
                }
            }
        }
    }
}

bool AtomEngineServer::load()
{
    Logger::info() << "Initialization ...";
    QFile file(backupFileName);
    if(file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString command = stream.readLine();
            QJsonDocument doc = QJsonDocument::fromJson(command.toUtf8());
            if (doc.isObject()) {
                QJsonObject req = doc.object();
                QString commandName = req["command"].toString();
                if (commandName == "create_order") {
                    QJsonObject orderJson = req["order"].toObject();
                    QString key = "";
                    if (req.contains("key")) {
                        key = req["key"].toString();
                    }
                    createOrder(key, orderJson);
                }
                if (commandName == "delete_order") {
                    long long id = req["id"].toVariant().toLongLong();
                    QString key = "";
                    if (req.contains("key")) {
                        key = req["key"].toString();
                    }
                    deleteOrder(key, id);
                }
                if (commandName == "create_trade") {
                    long long orderId = req["orderId"].toVariant().toLongLong();
                    QString initiatorAddr = req["address"].toString();
                    QString key = "";
                    if (req.contains("key")) {
                        key = req["key"].toString();
                    }
                    createTrade(key, orderId, initiatorAddr);
                }
                if (commandName == "update_trade") {
                    QJsonObject tradeJson = req["trade"].toObject();
                    QString key = "";
                    if (req.contains("key")) {
                        key = req["key"].toString();
                    }
                    updateTrade(key, tradeJson);
                }
            }
        }
        file.close();
        Logger::info() << "Load engine data success";
        return true;
    } else {
        Logger::info() << "Load engine data failed";
        return false;
    }
}

void AtomEngineServer::saveCommand(QJsonDocument& doc)
{
    QByteArray docByteArray = doc.toJson(QJsonDocument::Compact);
    QString strJson = QLatin1String(docByteArray);
    if (backupFile_.open(QIODevice::Append)) {
        QTextStream stream(&backupFile_);
        stream << strJson << endl;
        backupFile_.close();
    } else {
        Logger::info() << "failed to save command: " + strJson;
    }
}

OrderInfoPtr AtomEngineServer::createOrder(const QString& key, const QJsonObject& orderJson)
{
    ++curOrderId_;
    OrderInfoPtr order = std::make_shared<OrderInfo>(curOrderId_, orderJson);
    order->sign(key);
    orders_[curOrderId_] = order;
    return order;
}

bool AtomEngineServer::deleteOrder(const QString& key, long long id)
{
    auto it = orders_.find(id);
    if (it != orders_.end() && it->second->checkKey(key)) {
        orders_.erase(it);
        return true;
    } else {
        return false;
    }
}

TradeInfoPtr AtomEngineServer::createTrade(const QString& key, long long orderId, const QString& initiatorAddress)
{
    auto it = orders_.find(orderId);
    if (it != orders_.end()) {
        OrderInfoPtr order = it->second;
        orders_.erase(it);
        ++curTradeId_;
        TradeInfoPtr trade = std::make_shared<TradeInfo>(curTradeId_, order, initiatorAddress);
        trade->sign(key);
        trades_[curTradeId_] = trade;
        return trade;
    } else {
        return TradeInfoPtr();
    }
}

TradeInfoPtr AtomEngineServer::updateTrade(const QString& key, const QJsonObject& tradeJson)
{
    long long id = tradeJson["id"].toVariant().toLongLong();
    auto it = trades_.find(id);
    if (it != trades_.end() && (it->second->checkKey(key) || it->second->checkOrderKey(key))) {
        TradeInfoPtr trade = it->second;
        trade->secretHash_ = tradeJson["secretHash"].toString();
        trade->contractInitiator_ = tradeJson["contractInitiator"].toString();
        trade->contractParticipant_ = tradeJson["contractParticipant"].toString();
        trade->initiatorContractTransaction_ = tradeJson["initiatorContractTransaction"].toString();
        trade->participantContractTransaction_ = tradeJson["participantContractTransaction"].toString();
        trade->initiatorRedemptionTransaction_ = tradeJson["initiatorRedemptionTransaction"].toString();
        trade->participantRedemptionTransaction_ = tradeJson["participantRedemptionTransaction"].toString();
        if (!trade->initiatorCommissionPaid_) {
            trade->initiatorCommissionPaid_ = tradeJson["commissionInitiatorPaid"].toBool();
        }
        if (!trade->participantCommissionPaid_) {
            trade->participantCommissionPaid_ = tradeJson["commissionParticipantPaid"].toBool();
        }

        if (tradeJson.contains("refundedInit")) {
            trade->refundedInit_ = tradeJson["refundedInit"].toBool();
        }
        if (tradeJson.contains("refundedPart")) {
            trade->refundedPart_ = tradeJson["refundedPart"].toBool();
        }

        if (tradeJson.contains("refundTimeInit")) {
            trade->refundTimeInit_ = tradeJson["refundTimeInit"].toVariant().toLongLong();
        }

        if (tradeJson.contains("refundTimePart")) {
            trade->refundTimePart_ = tradeJson["refundTimePart"].toVariant().toLongLong();
        }

        return trade;
    } else {
        return TradeInfoPtr();
    }
}
