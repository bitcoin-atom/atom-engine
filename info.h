// Copyright 2018 AtomicSwap Solutions Ltd. All rights reserved.
// Use of this source code is governed by Microsoft Reference Source
// License (MS-RSL) that can be found in the LICENSE file.

#ifndef INFO_H
#define INFO_H

#include <memory>
#include <QString>
#include <QJsonObject>

struct OrderInfo;
typedef std::shared_ptr<OrderInfo> OrderInfoPtr;

struct OrderInfo {
    OrderInfo(OrderInfoPtr reverceOrder, const QString& getAddress);
    OrderInfo(long long orderId, const QJsonObject& order);
    long long orderId_;
    QString sendCur_;
    long long sendCount_;
    QString getCur_;
    long long getCount_;
    QString getAddress_;

    QString getJson() const;
};

struct TradeInfo {
    TradeInfo(long long tradeId, OrderInfoPtr order, const QString& initiatorAddress) :
        tradeId_(tradeId),
        order_(order),
        initiatorAddress_(initiatorAddress),
        initiatorCommissionPaid_(false),
        participantCommissionPaid_(false)
    {}

    long long tradeId_;

    OrderInfoPtr order_;
    QString initiatorAddress_;

    QString secretHash_;

    QString contractInitiator_;
    QString contractParticipant_;

    QString initiatorContractTransaction_;
    QString participantContractTransaction_;

    QString initiatorRedemptionTransaction_;
    QString participantRedemptionTransaction_;

    bool initiatorCommissionPaid_;
    bool participantCommissionPaid_;

    QString getJson() const;
};

#endif // INFO_H
