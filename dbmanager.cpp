#include "dbmanager.h"
#include "logger.h"
#include "info.h"
#include <QVariant>

DBManager::DBManager()
{

}

DBManager::~DBManager()
{

}

DBManager& DBManager::instance()
{
    static DBManager dbManager;
    return dbManager;
}

bool DBManager::init(const QString& dbName)
{
    Logger::info() << "Database initialization ...";
    Logger::info() << "DB name = " + dbName;

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbName);
    bool res = db.open();

    if (!res) {
        Logger::info() << "Failed to open database";
    } else {
        queryAddToOrders.prepare("INSERT INTO orders (id, sendCur, sendCount, getCur, getCount, getAddress, hash) " \
                                 "VALUES (:id, :sendCur, :sendCount, :getCur, :getCount, :getAddress, :hash)");

        queryAddToTrades.prepare("INSERT INTO trades (orderId, sendCur, sendCount, getCur, getCount, getAddress, orderHash," \
                                 "id, initiatorAddress, secretHash, contractInitiator, contractParticipant, initiatorContractTransaction, " \
                                 "participantContractTransaction, initiatorRedemptionTransaction, participantRedemptionTransaction," \
                                 "initiatorCommissionPaid, participantCommissionPaid," \
                                 "refundedInit, refundedPart, refundTimeInit, refundTimePart, hash) " \
                                 "VALUES (:orderId, :sendCur, :sendCount, :getCur, :getCount, :getAddress, :orderHash," \
                                 ":id, :initiatorAddress, :secretHash, :contractInitiator, :contractParticipant, :initiatorContractTransaction, " \
                                 ":participantContractTransaction, :initiatorRedemptionTransaction, :participantRedemptionTransaction," \
                                 ":initiatorCommissionPaid, :participantCommissionPaid," \
                                 ":refundedInit, :refundedPart, :refundTimeInit, :refundTimePart, :hash)");

        queryAddToBL.prepare("INSERT INTO black_list (ip) VALUES (:ip)");

        queryDeleteFromOrders.prepare("DELETE FROM orders WHERE id=:id");

        queryDeleteFromTrades.prepare("DELETE FROM trades WHERE id=:id");

        queryUpdateTrade.prepare("UPDATE SET orderId=:orderId, sendCur=:sendCur, sendCount=:sendCount, getCur=:getCur, getCount=:getCount, getAddress=:getAddress, orderHash=:orderHash, " \
                                 "initiatorAddress=:initiatorAddress, secretHash=:secretHash, contractInitiator=:contractInitiator, " \
                                 "contractParticipant=:contractParticipant, initiatorContractTransaction=:initiatorContractTransaction, participantContractTransaction=:participantContractTransaction, " \
                                 "initiatorRedemptionTransaction=:initiatorRedemptionTransaction, participantRedemptionTransaction=:participantRedemptionTransaction, " \
                                 "initiatorCommissionPaid=:initiatorCommissionPaid, participantCommissionPaid=:participantCommissionPaid," \
                                 "refundedInit=:refundedInit, refundedPart=:refundedPart, refundTimeInit=:refundTimeInit, refundTimePart=:refundTimePart, hash=:hash " \
                                 "WHERE id=:id");

        queryLoadOrders.prepare("SELECT * FROM orders");

        queryLoadTrades.prepare("SELECT * FROM trades");

        queryLoadBL.prepare("SELECT * FROM black_list");
    }

    return res;
}

void DBManager::addToOrders(OrderInfoPtr order)
{
    queryAddToOrders.bindValue(":id", order->orderId_);
    queryAddToOrders.bindValue(":sendCur", order->sendCur_);
    queryAddToOrders.bindValue(":sendCount", order->sendCount_);
    queryAddToOrders.bindValue(":getCur", order->getCur_);
    queryAddToOrders.bindValue(":getCount", order->getCount_);
    queryAddToOrders.bindValue(":getAddress", order->getAddress_);
    queryAddToOrders.bindValue(":hash", order->getHash());
    queryAddToOrders.exec();
}

void DBManager::addToTrades(TradeInfoPtr trade)
{
    queryAddToTrades.bindValue(":orderId", trade->order_->orderId_);
    queryAddToTrades.bindValue(":sendCur", trade->order_->sendCur_);
    queryAddToTrades.bindValue(":sendCount", trade->order_->sendCount_);
    queryAddToTrades.bindValue(":getCur", trade->order_->getCur_);
    queryAddToTrades.bindValue(":getCount", trade->order_->getCount_);
    queryAddToTrades.bindValue(":getAddress", trade->order_->getAddress_);
    queryAddToTrades.bindValue(":orderHash", trade->order_->getHash());

    queryAddToTrades.bindValue(":id", trade->tradeId_);
    queryAddToTrades.bindValue(":initiatorAddress", trade->initiatorAddress_);
    queryAddToTrades.bindValue(":secretHash", trade->secretHash_);
    queryAddToTrades.bindValue(":contractInitiator", trade->contractInitiator_);
    queryAddToTrades.bindValue(":contractParticipant", trade->contractParticipant_);
    queryAddToTrades.bindValue(":initiatorContractTransaction", trade->initiatorContractTransaction_);
    queryAddToTrades.bindValue(":participantContractTransaction", trade->participantContractTransaction_);
    queryAddToTrades.bindValue(":initiatorRedemptionTransaction", trade->initiatorRedemptionTransaction_);
    queryAddToTrades.bindValue(":participantRedemptionTransaction", trade->participantRedemptionTransaction_);
    queryAddToTrades.bindValue(":initiatorCommissionPaid", trade->initiatorCommissionPaid_);
    queryAddToTrades.bindValue(":participantCommissionPaid", trade->participantCommissionPaid_);
    queryAddToTrades.bindValue(":refundedInit", trade->refundedInit_);
    queryAddToTrades.bindValue(":refundedPart", trade->refundedPart_);
    queryAddToTrades.bindValue(":refundTimeInit", trade->refundTimeInit_);
    queryAddToTrades.bindValue(":refundTimePart", trade->refundTimePart_);
    queryAddToTrades.bindValue(":hash", trade->getHash());

    queryAddToTrades.exec();
}

void DBManager::addToBlackList(const QString& blackListIP)
{
    queryAddToBL.bindValue(":ip", blackListIP);
    queryAddToBL.exec();
}

void DBManager::deleteFromOrders(long long orderId)
{
    queryDeleteFromOrders.bindValue(":id", orderId);
    queryDeleteFromOrders.exec();
}

void DBManager::deleteFromTrades(long long tradeId)
{
    queryDeleteFromTrades.bindValue(":id", tradeId);
    queryDeleteFromTrades.exec();
}

void DBManager::updateTrade(TradeInfoPtr trade)
{
    queryUpdateTrade.bindValue(":orderId", trade->order_->orderId_);
    queryUpdateTrade.bindValue(":sendCur", trade->order_->sendCur_);
    queryUpdateTrade.bindValue(":sendCount", trade->order_->sendCount_);
    queryUpdateTrade.bindValue(":getCur", trade->order_->getCur_);
    queryUpdateTrade.bindValue(":getCount", trade->order_->getCount_);
    queryUpdateTrade.bindValue(":getAddress", trade->order_->getAddress_);
    queryUpdateTrade.bindValue(":orderHash", trade->order_->getHash());

    queryUpdateTrade.bindValue(":id", trade->tradeId_);
    queryUpdateTrade.bindValue(":initiatorAddress", trade->initiatorAddress_);
    queryUpdateTrade.bindValue(":secretHash", trade->secretHash_);
    queryUpdateTrade.bindValue(":contractInitiator", trade->contractInitiator_);
    queryUpdateTrade.bindValue(":contractParticipant", trade->contractParticipant_);
    queryUpdateTrade.bindValue(":initiatorContractTransaction", trade->initiatorContractTransaction_);
    queryUpdateTrade.bindValue(":participantContractTransaction", trade->participantContractTransaction_);
    queryUpdateTrade.bindValue(":initiatorRedemptionTransaction", trade->initiatorRedemptionTransaction_);
    queryUpdateTrade.bindValue(":participantRedemptionTransaction", trade->participantRedemptionTransaction_);
    queryUpdateTrade.bindValue(":initiatorCommissionPaid", trade->initiatorCommissionPaid_);
    queryUpdateTrade.bindValue(":participantCommissionPaid", trade->participantCommissionPaid_);
    queryUpdateTrade.bindValue(":refundedInit", trade->refundedInit_);
    queryUpdateTrade.bindValue(":refundedPart", trade->refundedPart_);
    queryUpdateTrade.bindValue(":refundTimeInit", trade->refundTimeInit_);
    queryUpdateTrade.bindValue(":refundTimePart", trade->refundTimePart_);
    queryUpdateTrade.bindValue(":hash", trade->getHash());

    queryUpdateTrade.exec();
}

void DBManager::loadOrders(Orders& orders)
{
    queryLoadOrders.exec();

    while (queryLoadOrders.next())
    {
        long long id = queryLoadOrders.value(0).toLongLong();
        OrderInfoPtr order = std::make_shared<OrderInfo>();
        order->orderId_ = id;
        order->sendCur_ = queryLoadOrders.value(1).toString();
        order->sendCount_ = queryLoadOrders.value(2).toLongLong();
        order->getCur_ = queryLoadOrders.value(3).toString();
        order->getCount_ = queryLoadOrders.value(4).toLongLong();
        order->getAddress_ = queryLoadOrders.value(5).toString();
        order->setHash(queryLoadOrders.value(6).toString());
        orders[id] = order;
    }
}

void DBManager::loadTrades(Trades& trades)
{
    queryLoadTrades.exec();

    while (queryLoadTrades.next())
    {
        long long id = queryLoadTrades.value(7).toLongLong();
        TradeInfoPtr trade = std::make_shared<TradeInfo>();
        trade->order_ = std::make_shared<OrderInfo>();
        trade->order_->orderId_ = queryLoadTrades.value(0).toLongLong();
        trade->order_->sendCur_ = queryLoadTrades.value(1).toString();
        trade->order_->sendCount_ = queryLoadTrades.value(2).toLongLong();
        trade->order_->getCur_ = queryLoadTrades.value(3).toString();
        trade->order_->getCount_ = queryLoadTrades.value(4).toLongLong();
        trade->order_->getAddress_ = queryLoadTrades.value(5).toString();
        trade->order_->setHash(queryLoadTrades.value(6).toString());
        trade->tradeId_ = id;
        trade->initiatorAddress_ = queryLoadTrades.value(8).toString();
        trade->secretHash_ = queryLoadTrades.value(9).toString();
        trade->contractInitiator_ = queryLoadTrades.value(10).toString();
        trade->contractParticipant_ = queryLoadTrades.value(11).toString();
        trade->initiatorContractTransaction_ = queryLoadTrades.value(12).toString();
        trade->participantContractTransaction_ = queryLoadTrades.value(13).toString();
        trade->initiatorRedemptionTransaction_ = queryLoadTrades.value(14).toString();
        trade->participantRedemptionTransaction_ = queryLoadTrades.value(15).toString();
        trade->initiatorCommissionPaid_ = queryLoadTrades.value(16).toBool();
        trade->participantCommissionPaid_ = queryLoadTrades.value(17).toBool();
        trade->refundedInit_ = queryLoadTrades.value(18).toBool();
        trade->refundedPart_ = queryLoadTrades.value(19).toBool();
        trade->refundTimeInit_ = queryLoadTrades.value(20).toLongLong();
        trade->refundTimePart_ = queryLoadTrades.value(21).toLongLong();
        trade->setHash(queryLoadTrades.value(22).toString());
        trades[id] = trade;
    }
}

void DBManager::loadBlackList(BlackList& blackList)
{
    queryLoadBL.exec();

    while (queryLoadBL.next())
    {
        blackList.insert(queryLoadBL.value(0).toString());
    }
}
