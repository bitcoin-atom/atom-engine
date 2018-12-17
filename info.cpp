// Copyright 2018 AtomicSwap Solutions Ltd. All rights reserved.
// Use of this source code is governed by Microsoft Reference Source
// License (MS-RSL) that can be found in the LICENSE file.

#include "info.h"

OrderInfo::OrderInfo(OrderInfoPtr reverceOrder, const QString& getAddress) :
    orderId_(0),
    sendCur_(reverceOrder->getCur_),
    sendCount_(reverceOrder->getCount_),
    getCur_(reverceOrder->sendCur_),
    getCount_(reverceOrder->sendCount_),
    getAddress_(getAddress)
{

}

OrderInfo::OrderInfo(long long orderId, const QJsonObject& order) :
    orderId_(orderId)
{
    sendCur_ = order["sendCur"].toString();
    sendCount_ = order["sendCount"].toInt();
    getCur_ = order["getCur"].toString();
    getCount_ = order["getCount"].toInt();
    getAddress_ = order["getAddr"].toString();
}

QString OrderInfo::getJson() const
{
    QString res = "{\"sendCur\": \"" + sendCur_ + "\", \"getCur\": \"" + getCur_ + "\", \"sendCount\": " + QString::number(sendCount_) + ", \"getCount\": " + QString::number(getCount_) + ", \"getAddr\": \"" + getAddress_ + "\", \"id\": " + QString::number(orderId_) + "}";
    return res;
}

QString TradeInfo::getJson() const
{
    QString paidInit = "false";
    if (initiatorCommissionPaid_) {
        paidInit = "true";
    }

    QString paidPart = "false";
    if (participantCommissionPaid_) {
        paidPart = "true";
    }

    QString res = "{" \
                    "\"id\": " + QString::number(tradeId_) + ", " \
                    "\"order\": " + order_->getJson() + ", " \
                    "\"initiatorAddr\": \"" + initiatorAddress_ + "\", " \
                    "\"secretHash\": \"" + secretHash_ + "\", " \
                    "\"contractInitiator\": \"" + contractInitiator_ + "\", " \
                    "\"contractParticipant\": \"" + contractParticipant_ + "\", " \
                    "\"initiatorContractTransaction\": \"" + initiatorContractTransaction_ + "\", " \
                    "\"participantContractTransaction\": \"" + participantContractTransaction_ + "\", " \
                    "\"initiatorRedemptionTransaction\": \"" + initiatorRedemptionTransaction_ + "\", " \
                    "\"participantRedemptionTransaction\": \"" + participantRedemptionTransaction_ + "\", " \
                    "\"commissionInitiatorPaid\": " + paidInit + ", " \
                    "\"commissionParticipantPaid\": " + paidPart + "}";
    return res;
}
