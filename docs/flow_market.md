# 行情推送流程 (Market Data Flow)

## 1. 流程概览

行情数据的链路要求极高的低延迟特性。系统从 CTP 接口接收原始 Ticks，经过核心层极简处理后，立即通过 ZeroMQ 广播给前端。

**链路图**:
`Exchange -> CTP API -> MdHandler -> Publisher -> ZMQ(PUB) -> ZMQ(SUB) -> ZmqWorker -> MarketModel -> Qt UI`

## 2. 详细步骤

### 2.1 核心层接收 (Core Side)

**文件**: `ctp_core/src/api/MdHandler.cpp`

1.  **Callback**: CTP 触发 `OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pData)`。
2.  **数据清洗**: 
    - 过滤非法价格 (如 `DBL_MAX`)。
    - 构造内部统一结构 `TickData`。
    - 填充字段：`LastPrice`, `Volume`, `OpenInterest`, `Bid/Ask`, `Upper/LowerLimit`.
3.  **高性能发布**:
    - 调用 `Publisher::publishTick(TickData&)`。

### 2.2 核心层发布 (Publisher Optimization)

**文件**: `ctp_core/src/network/Publisher.cpp`

**关键优化**: 为了极致性能，此处**不使用** `nlohmann::json` 对象构建，而是直接使用 `snprintf` 进行字符串格式化。
- **原因**: 高频行情下（每秒数十上百笔），JSON对象的内存分配和构建开销会导致微秒级延迟累积和CPU抖动。
- **实现**: 在栈上分配 `char buffer[2048]`，零堆内存分配。

```cpp
// 示例 (Publisher.cpp)
char buffer[2048];
int len = snprintf(buffer, sizeof(buffer),
    "{\"instrument_id\":\"%s\",\"last_price\":%.8g,...}", ...);
publisher_->send(topic, sndmore);
publisher_->send(buffer, none);
```

### 2.3 传输层 (ZMQ)

- **Topic**: `QuantLabs::zmq_topics::MARKET_DATA` ("MD")
- **Transport**: TCP Loopback (127.0.0.1) 或 IPC。

### 2.4 前端接收 (Qt Side)

**文件**: `qt_manager/src/network/ZmqWorker.cpp`

1.  **监听**: `ZmqWorker` 在后台线程轮询 ZMQ socket。
2.  **解析**: `onRead()` 收到 "MD" 主题消息。
    - 此时才解析 JSON 字符串为 `QJsonObject`。
3.  **信号发射**: `emit tickReceived(QString instrument, QJsonObject data)`。

### 2.5 界面渲染 (Qt UI)

**文件**: `qt_manager/src/models/MarketModel.cpp`

1.  **数据更新**: `MarketModel::updateTick` 被槽函数调用。
2.  **智能刷新**:
    - 查找内部 `QHash<QString, MarketItem> m_data`。
    - 仅更新变更字段（LastPrice, Volume 等）。
    - 计算涨跌幅 (Chg% = (Last - PreClose) / PreClose)。
3.  **视图通知**: 调用 `emit dataChanged(...)` 通知 QML。
    - QML 中的 `TableView` 或 `ListView` 仅重绘受影响的单元格，实现流畅的高频跳动效果。

## 3. 涉及的数据结构

核心 tick 数据包含但不限于：
- **InstrumentID**: 合约代码
- **LastPrice**: 最新价
- **Volume**: 成交量
- **OpenInterest**: 持仓量
- **BidPrice1/AskPrice1**: 买一/卖一价
- **BidVolume1/AskVolume1**: 买一/卖一量
- **UpdateTime/Millisec**: 更新时间
