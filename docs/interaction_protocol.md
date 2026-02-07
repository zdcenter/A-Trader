# 前后端交互协议 (Interaction Protocol)

## 1. 通信架构

- **Transport**: ZeroMQ (TCP Loopback)
- **Serialization**: JSON (via `nlohmann/json` & Qt `QJsonObject`)
- **Address**:
  - **PUB (Core -> Qt)**: `tcp://*:5555`
  - **REP (Qt -> Core)**: `tcp://*:5556`

## 2. Topic 定义 (PUB/SUB)

| Topic String | 常量名 (`zmq_topics`) | 用途 | 数据结构示例 |
| :--- | :--- | :--- | :--- |
| **MD_BIN** | `MARKET_DATA_BIN` | 实时行情 (Binary Only) | `struct TickData` |
| **POS** | `POSITION_DATA` | 持仓更新 | `{"instrument_id":"rb2505","direction":"0","position":5,...}` |
| **ORD** | `ORDER_DATA` | 委托回报 | `{"order_sys_id":"123","status":"0",...}` |
| **TRD** | `TRADE_DATA` | 成交回报 | `{"trade_id":"T001","price":3600,...}` |
| **ACC** | `ACCOUNT_DATA` | 资金账户 | `{"balance":100000,"available":50000,...}` |
| **INS** | `INSTRUMENT_DATA` | 合约信息 | `{"instrument_id":"rb2505","price_tick":1.0,...}` |
| **STR** | `TOPIC_STRATEGY` | 策略/条件单 | `{"type":"RTN_COND","data":{...}}` |

## 3. 指令集 (REQ/REP)

所有请求必须包含 `type` 字段。

### 3.1 基础指令

- **心跳 (Ping)**
  - Req: `{"type": "PING"}`
  - Rep: `{"type": "PONG"}`

- **同步状态 (Sync)**
  - Req: `{"type": "CMD_SYNC_STATE"}`
  - Rep: `{"status": "ok", "msg": "Sync started"}`
  - *注：此指令触发 Core 推送全量 POS/ORD/TRD/ACC 数据*

### 3.2 交易指令

- **报单 (Order)**
  - Req:
    ```json
    {
      "type": "CMD_ORDER",
      "id": "rb2505",
      "price": 3600.0,
      "vol": 1,
      "dir": "0", // 0:Buy, 1:Sell
      "off": "0", // 0:Open, 1:Close, 3:CloseToday
      "price_type": "2" // 1:Any, 2:Limit
    }
    ```
  - Rep: `{"status": "ok", "msg": "..."}` 或 `{"status": "error"}`

- **撤单 (Action)**
  - Req:
    ```json
    {
      "type": "CMD_ORDER_ACTION",
      "data": {
        "InstrumentID": "rb2505",
        "ExchangeID": "SHFE",
        "OrderSysID": "      123456" // Space padded
      }
    }
    ```

### 3.3 策略/条件单指令

- **新增条件单** (`CMD_COND_INSERT`)
- **撤销条件单** (`CMD_COND_CANCEL`)
- **修改条件单** (`CMD_COND_MODIFY`)
- **查询条件单** (`CMD_COND_QUERY`)

## 4. 类型映射

### 方向 (Direction)
- `'0'` / `"buy"`: 买 (Long)
- `'1'` / `"sell"`: 卖 (Short)

### 开平 (Offset)
- `'0'` / `"open"`: 开仓
- `'1'` / `"close"`: 平仓
- `'3'` / `"close_today"`: 平今
- `'4'` / `"close_yesterday"`: 平昨

### 报单状态 (OrderStatus)
- `'0'`: 全部成交
- `'1'`: 部分成交还在队
- `'3'`: 未成交还在队
- `'5'`: 撤单
- `'a'`: 未知
