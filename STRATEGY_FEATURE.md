# 策略选择功能实现总结

## 1. 功能概述
已完成全局策略选择功能的开发。用户可以在下单面板选择当前策略（如 "Default", "Manual", "Grid" 等），该选择会被持久化，并在每次下单时自动关联。

## 2. 修改内容

### 后端 (CTP Core)
*   **持久化存储**: 在数据库 `tb_settings` 表中存储 `current_strategy_id`。
*   **自动加载**: 用户登录成功后，自动从数据库加载上次选中的策略 ID。
*   **下单逻辑 (`insertOrder`)**: 
    *   如果下单指令包含 `strategy_id`，则使用该 ID。
    *   如果未包含（旧接口调用），则自动使用当前全局选中的策略 ID。
*   **命令扩展**: 新增 ZMQ 命令 `SET_STRATEGY`，允许前端修改当前策略。

### 前端 (Qt Manager)
*   **UI 界面**: 
    *   `OrderPanel.qml` 和 `ConditionOrderPanel.qml` 均使用统一的深色主题策略选择下拉框。
    *   **数据源**: 均绑定 `OrderController.strategyList`。
*   **状态同步**: 
    *   两个面板共享全局的 `OrderController.currentStrategy`。
    *   在任意面板切换策略，另一个面板会自动同步更新。
    *   **持久化**: `OrderPanel` 负责记录 `lastStrategyId`，启动时自动恢复。

## 3. 验证方法

1.  **启动 CTP Core**:
    ```bash
    cd /home/zd/A-Trader/ctp_core
    ./build/ctp_core
    ```
2.  **启动 Qt Manager**:
    ```bash
    cd /home/zd/A-Trader/qt_manager
    ./build/qt_manager
    ```
3.  **测试步骤**:
    *   在左侧下单面板，"策略归属" 下拉框选择或输入 "MyStrategy"。
    *   检查 CTP Core 日志，应看到 `[Td] Set Current Strategy => MyStrategy`。
    *   发送一笔报单。
    *   检查 CTP Core 日志，应看到 `[Td] Insert Order Strategy: MyStrategy`（或者在数据库 `tb_orders` 中验证 `strategy_id` 字段）。
    *   重启 Qt Manager，确认 "策略归属" 自动恢复为 "MyStrategy"。
