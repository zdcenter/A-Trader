#pragma once

#include <QObject>
#include <QString>
#include <nlohmann/json.hpp>

namespace atrad {

class AccountInfo : public QObject {
    Q_OBJECT
    Q_PROPERTY(double balance READ balance NOTIFY changed)
    Q_PROPERTY(double available READ available NOTIFY changed)
    Q_PROPERTY(double margin READ margin NOTIFY changed)
    Q_PROPERTY(double frozen READ frozen NOTIFY changed)
    Q_PROPERTY(double commission READ commission NOTIFY changed)
    Q_PROPERTY(double floatingProfit READ floatingProfit NOTIFY changed)
    Q_PROPERTY(double equity READ equity NOTIFY changed)

public:
    explicit AccountInfo(QObject *parent = nullptr) : QObject(parent) {}

    double balance() const { return _balance; }
    double available() const { return _available; }
    double margin() const { return _margin; }
    double frozen() const { return _frozen; }
    double commission() const { return _commission; }
    double floatingProfit() const { return _floating_profit; }
    double equity() const { return _balance + _floating_profit; }

public slots:
    void updateAccount(const QString& json) {
        try {
            auto j = nlohmann::json::parse(json.toStdString());
            _balance = j["bal"];
            _available = j["avail"];
            _margin = j["margin"];
            _frozen = j["frozen"];
            _commission = j["comm"];
            emit changed();
        } catch (...) {}
    }

    void setFloatingProfit(double profit) {
        if (std::abs(_floating_profit - profit) > 0.01) {
            _floating_profit = profit;
            emit changed();
        }
    }

signals:
    void changed();

private:
    double _balance = 0.0;
    double _available = 0.0;
    double _margin = 0.0;
    double _frozen = 0.0;
    double _commission = 0.0;
    double _floating_profit = 0.0;
};

} // namespace atrad
