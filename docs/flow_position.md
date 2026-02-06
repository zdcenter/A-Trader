# 持仓同步与计算流程 (Position Flow)

## 1. 核心挑战

CTP 的持仓查询 (`QryInvestorPosition`) 返回的是汇总数据（总持仓、昨仓、今仓），而**持仓明细** (`QryInvestorPositionDetail`) 返回每一笔开仓的剩余情况。

为了准确计算**持仓均价**（尤其是上期所昨仓需按结算价计算）以及支持**FIFO（先开先平）**逻辑，本系统采用了**"基于明细重算汇总"**的策略。

## 2. 同步流程 (Sync)

### 2.1 触发查询
**场景**: 每日启动或断线重连时。
1.  **Frontend**: 发送 `CMD_SYNC_STATE`。
2.  **Core**: `TraderHandler::qryPosition()`。

### 2.2 两阶段查询 (Core)
**文件**: `ctp_core/src/api/TraderHandler.cpp`

1.  **阶段一**: `ReqQryInvestorPosition`。
    - 收到回报 `OnRspQryInvestorPosition`。
    - **注意**: 此时**不**直接更新持仓缓存。仅利用返回的 InstrumentID 触发费率查询 (`queueRateQuery`)，确保后续计算手续费或盈亏有数据支撑。
    - **完全接收后**: 触发 `qryPositionDetail()`。

2.  **阶段二**: `ReqQryInvestorPositionDetail`。
    - 清空内部明细队列 `open_position_queues_`。
    - 收到回报 `OnRspQryInvestorPositionDetail`。
    - 将每条明细按 `InstrumentID + Direction` 分组存入队列。

### 2.3 本地重算 (Recalculation)

当阶段二全部完成后 (`bIsLast=true`)，`TraderHandler` 执行重算逻辑：

1.  **排序**: 对每个合约的明细队列按 `OpenDate` ASC, `TradeID` ASC 排序 (FIFO 顺序)。
2.  **聚合计算**: 遍历队列，累加 `Volume`。
    - **今仓 (Today)**: `OpenDate == TradingDay`
    - **昨仓 (Yd)**: `OpenDate != TradingDay`
3.  **成本计算 (PositionCost)**:
    - **非 SHFE/INE**: `OpenPrice * Volume * Multiplier`
    - **SHFE/INE (昨仓)**: `PreSettlementPrice * Volume * Multiplier` (逐日盯市逻辑)
    - **SHFE/INE (今仓)**: `OpenPrice * Volume * Multiplier`
4.  **缓存更新**: 生成最终的 `PositionData` 并存入 `position_cache_`。

### 2.4 推送前端

- **推送**: CTP Core 调用 `Publisher::publishPosition` -> ZMQ PUB `POSITION_DATA`。
- **快照机制**: 推送时通过 `snapshot_seq` 标记这是一次全量刷新，前端收到后可以清空旧数据或进行 Diff 更新。

## 3. 实时更新 (Realtime)

除了查询同步，系统还利用成交回报 (`OnRtnTrade`) 进行实时持仓模拟更新（可选），但在本架构中，为了保持绝对准确，我们通常依赖 CTP 的推送触发定期查询，或者依靠以下机制：

- **成交驱动**: 收到 `OnRtnTrade` 后，立即更新内存中的持仓数量和成本，并推送 Diff。
- **定时同步**: 定时器每隔 N 秒触发一次完整查询 (QryPosition)，修正可能的误差。

（当前代码实现主要依赖**启动同步** + **成交后手动/自动触发查询**的模式保持数据一致性）
