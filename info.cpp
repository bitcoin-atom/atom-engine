// Copyright 2018 AtomicSwap Solutions Ltd. All rights reserved.
// Use of this source code is governed by Microsoft Reference Source
// License (MS-RSL) that can be found in the LICENSE file.

#include "info.h"
#include <QVariant>
#include <QCryptographicHash>

OrderInfo::OrderInfo(long long orderId, const QJsonObject& order) :
    orderId_(orderId)
{
    sendCur_ = order["sendCur"].toString();
    sendCount_ = order["sendCount"].toVariant().toLongLong();
    getCur_ = order["getCur"].toString();
    getCount_ = order["getCount"].toVariant().toLongLong();
    getAddress_ = order["getAddr"].toString();
}

QString OrderInfo::getJson() const
{
    QString res = "{\"sendCur\": \"" + sendCur_ + "\", \"getCur\": \"" + getCur_ + "\", \"sendCount\": " + QString::number(sendCount_) + ", \"getCount\": " + QString::number(getCount_) + ", \"getAddr\": \"" + getAddress_ + "\", \"id\": " + QString::number(orderId_) + "}";
    return res;
}

void OrderInfo::sign(const QString& key)
{
    if (key != "") {
        keyHash_ = QString(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex());
    } else {
        keyHash_ = "";
    }
}

bool OrderInfo::checkKey(const QString& key)
{
    if (keyHash_ == "") {
        return true;
    }

    if (key == "") {
        return false;
    }

    QString keyHash = QString(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex());
    return keyHash_ == keyHash ? true : false;
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

    QString refundedInitStr = "false";
    if (refundedInit_) {
        refundedInitStr = "true";
    }
    QString refundedPartStr = "false";
    if (refundedPart_) {
        refundedPartStr = "true";
    }

    QString res = "{" \
                    "\"id\": " + QString::number(tradeId_) + ", " \
                    "\"refundedInit\": " + refundedInitStr + ", " \
                    "\"refundedPart\": " + refundedPartStr + ", " \
                    "\"refundTimeInit\": " + QString::number(refundTimeInit_) + ", " \
                    "\"refundTimePart\": " + QString::number(refundTimePart_) + ", " \
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

void TradeInfo::sign(const QString& key)
{
    if (key != "") {
        keyHash_ = QString(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex());
    } else {
        keyHash_ = "";
    }
}

bool TradeInfo::checkKey(const QString& key)
{
    if (keyHash_ == "") {
        return true;
    }

    if (key == "") {
        return false;
    }

    QString keyHash = QString(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex());
    return keyHash_ == keyHash ? true : false;
}

bool TradeInfo::checkOrderKey(const QString& key)
{
    return order_->checkKey(key);
}
