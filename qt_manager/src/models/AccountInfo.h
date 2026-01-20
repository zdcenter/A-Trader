#pragma once

#include <QObject>
#include <QString>
#include <nlohmann/json.hpp>

#include "../../../shared/protocol/message_schema.h" // Relative path adjustment might be needed depending on include paths
// actually cmake usually sets include path. Let's try fully qualified or assuming include directories.
// The safe bet is to use the full path relative to project root or use configured include.
// In cmake: include_directories(${CMAKE_SOURCE_DIR}/../shared/protocol) ? No, let's look at CMakeLists.txt later.
// For now, let's use a relative path that works or assume it's in include path.
// Given file is /home/zd/A-Trader/qt_manager/src/models/AccountInfo.h
// shared is /home/zd/A-Trader/shared
// So ../../../shared/protocol/message_schema.h works.

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
    explicit AccountInfo(QObject *parent = nullptr) : QObject(parent) {
        std::memset(&_data, 0, sizeof(_data));
    }

    double balance() const { return _data.balance; }
    double available() const { return _data.available; }
    double margin() const { return _data.margin; }
    double frozen() const { return _data.frozen_margin; }
    double commission() const { return _data.commission; }
    double floatingProfit() const { return _floating_profit; }
    double equity() const { return _data.balance + _floating_profit; }

public slots:
    void updateAccount(const QString& json) {
        try {
            auto j = nlohmann::json::parse(json.toStdString());
            
            // Core now sends spec-compliant snake_case
            if (j.contains("balance")) _data.balance = j["balance"]; 
            else _data.balance = j.value("bal", 0.0);
            
            if (j.contains("available")) _data.available = j["available"]; 
            else _data.available = j.value("avail", 0.0);
            
            _data.margin = j.value("margin", 0.0);
            
            if (j.contains("frozen_margin")) _data.frozen_margin = j["frozen_margin"]; 
            else _data.frozen_margin = j.value("frozen", 0.0);
            
            if (j.contains("commission")) _data.commission = j["commission"]; 
            else _data.commission = j.value("comm", 0.0);
            
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
    AccountData _data;
    double _floating_profit = 0.0;
};

} // namespace atrad
