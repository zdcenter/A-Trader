# 系统架构与交互流程总览

本文档旨在帮助开发者深入理解 **QuantLabs A-Trader** 系统的内部运作机制，涵盖从底层 CTP 核心 (ctp_core) 到前端界面 (qt_manager) 的完整数据链路。

## 目录索引

1. [行情推送流程 (Market Data Flow)](./flow_market.md)
   - 核心如何接收交易所行情
   - 高频数据的高效序列化
   - 前端无需刷新的实时渲染机制

2. [交易指令全流程 (Order & Trade Flow)](./flow_order_trade.md)
   - 前端下单动作的生命周期
   - 核心层的高速 OrderRef 生成与路由
   - 报单回报与成交回报的异步推送与持久化
   - 撤单操作的处理逻辑

3. [持仓同步与计算流程 (Position Flow)](./flow_position.md)
   - 为什么需要"查询+明细重算"机制
   - 今仓、昨仓与持仓成本的计算逻辑
   - 前端快照与增量更新的处理

4. [前后端交互协议 (Interaction Protocol)](./interaction_protocol.md)
   - ZMQ 通信架构 (PUB/SUB 与 REQ/REP)
   - JSON 数据交换格式定义
   - 状态查询与同步指令

## 核心架构图解

```mermaid
graph TD
    subgraph "Frontend (Qt User Interface)"
        UI[Qt/QML UI Layer]
        WM[ZMQ Worker]
        OC[OrderController]
        Models[Market/Order/Pos Models]
    end

    subgraph "Communication Layer (ZeroMQ)"
        PUB[ZMQ PUB (Topic Broadcast)]
        REP[ZMQ REP (Command Response)]
    end

    subgraph "Backend (CTP Core)"
        Main[Main Dispatcher]
        MD[MdHandler (Market)]
        TD[TraderHandler (Trade)]
        DB[DBManager (PostgreSQL)]
        Pub[Publisher]
    end

    %% Data Flow
    MD -->|Ticks| Pub
    TD -->|Orders/Trades/Pos| Pub
    Pub -->|JSON| PUB
    PUB -->|Sub| WM
    WM -->|Signals| Models
    Models -->|Bindings| UI

    %% Control Flow
    UI -->|Action| OC
    OC -->|JSON Cmd| REP
    REP -->|Request| Main
    Main -->|Call| TD
    TD -->|CTP Req| Exchange
    TD -->|Data| DB
```

## 技术栈关键点

*   **CTP Core**: C++17, ZeroMQ (libzmq), PostgreSQL (libpqxx), nlohmann/json
*   **Qt Manager**: C++20, Qt6 (QML/Quick), ZeroMQ (cppzmq)
*   **通信协议**: 
    *   **PUB/SUB**: 用于高频数据单向推送（行情、状态变化）。
    *   **REQ/REP**: 用于低频控制指令（下单、撤单、查询）。

请点击上方链接查看各模块详细流程说明。
