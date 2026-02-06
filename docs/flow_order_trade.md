# 交易指令全流程 (Order & Trade Flow)

## 1. 流程概览

交易流程是双向的：前端发起指令 -> 核心处理 -> 交易所确认 -> 核心接收回报 -> 推送前端。

## 2. 下单流程 (Front to Back)

**场景**: 用户在界面点击"买入开仓"。

### 2.1 前端动作 (Qt)
**文件**: `qt_manager/src/models/OrderController.cpp`
1.  **用户操作**: QML 调用 `OrderController.sendOrder(id, price, vol, dir, off, type)`。
2.  **构建请求**: 创建 JSON 对象。
    ```json
    {
      "type": "CMD_ORDER",
      "id": "rb2505",
      "price": 3600,
      "vol": 1,
      "dir": "0", // Buy
      "off": "0", // Open
      "price_type": "2" // Limit
    }
    ```
3.  **发送**: 通过 `ZmqWorker` 的 `req_socket` 发送同步请求 (`sendRequest`)。

### 2.2 核心处理 (Core)
**文件**: `ctp_core/src/main.cpp`, `CommandServer.cpp`, `TraderHandler.cpp`

1.  **接收**: `CommandServer` 收到 REQ，回调 `main.cpp` 中的 Lambda。
2.  **路由**: `handlers[QuantLabs::CmdType::Order]` 被触发。
3.  **解析与执行**:
    - 解析 JSON 参数。
    - 调用 `TraderHandler::insertOrder`。
4.  **OrderRef 生成**:
    - **关键机制**: 为了支持高频报单，OrderRef 采用 `时间戳缓存(秒级) + 序列号` 方式生成，避免每次调用系统时间 API。
5.  **调用 CTP**: 填充 `CThostFtdcInputOrderField`，调用 `ReqOrderInsert`。
6.  **响应前端**: 立即返回 `{"status":"ok"}` 给前端，表示已提交（**注意：此时并未成交，甚至未被交易所确认**）。

## 3. 回报流程 (Back to Front)

**场景**: 报单被交易所接收，状态变更，或发生成交。

### 3.1 报单回报 (RtnOrder)
**文件**: `ctp_core/src/api/TraderHandler.cpp`

1.  **CTP 回调**: `OnRtnOrder(CThostFtdcOrderField *pOrder)`。
2.  **状态机**: 获取 `OrderStatus` (所有成交/部分成交/已撤单/未成交)。
3.  **持久化**: `DBManager::saveOrder(pOrder)` 异步写入 PostgreSQL `tb_orders`。
    - 关键字段：`OrderSysID` (交易所编号), `OrderRef` (本地引用)。
4.  **推送**: `Publisher::publishOrder` -> ZMQ PUB `ORDER_DATA`。

### 3.2 成交回报 (RtnTrade)
**文件**: `ctp_core/src/api/TraderHandler.cpp`

1.  **CTP 回调**: `OnRtnTrade(CThostFtdcTradeField *pTrade)`。
2.  **业务计算**:
    - **手续费**: 根据本地缓存的 `InstrumentData` (Rate) 计算预估手续费。
    - **平仓盈亏**: 若是平仓单，计算 `(OpenPrice - ClosePrice) * Volume * Multiplier`。
3.  **持久化**: `DBManager::saveTrade` 包含计算后的扩展字段。
4.  **推送**: `Publisher::publishTrade` -> ZMQ PUB `TRADE_DATA`。

### 3.3 前端更新 (Qt)
**文件**: `qt_manager/src/models/OrderModel.cpp`, `TradeModel.cpp`

1.  **接收**: `ZmqWorker` 收到 `ORDER_DATA` 或 `TRADE_DATA`。
2.  **OrderModel 更新**:
    - 这里的逻辑是 `Upsert`：如果 `OrderRef` 已存在，更新状态/成交量；如果不存在（比如其他客户端下的单），则新增。
    - 前端列表通常按时间倒序排列。
3.  **TradeModel 更新**:
    - 追加新成交记录。
    - 如果是平仓，可能会触发持仓列表的刷新。

## 4. 撤单流程

**路径**: `OrderController::cancelOrder` -> CMD `CMD_ORDER_ACTION` -> `TraderHandler::cancelOrder` -> CTP `ReqOrderAction`.

**关键点**:
- 必须提供 `ExchangeID` + `OrderSysID` (优先) 或 `FrontID` + `SessionID` + `OrderRef`。
- `TraderHandler` 会尝试自动补全 `ExchangeID` 并对 `OrderSysID` 进行右对齐补空格处理 (针对 SHFE/CFFEX 等交易所要求)。
