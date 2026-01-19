# QML UI 组件化重构说明

## 重构目标
将 `main.qml` 中的复杂 UI 部分拆分为独立的可复用组件，提高代码的可维护性和可读性。

## 新增组件

### 1. AccountInfoPanel.qml - 账户信息面板
**位置**: `/home/zd/A-Trader/qt_manager/qml/AccountInfoPanel.qml`

**功能**:
- 显示账户状态和账户 ID
- 显示动态权益、可用资金、持仓盈亏、占用保证金
- 显示 CTP 连接状态（已连接/未连接）
- 盈亏自动变色（红涨绿跌）

**对外属性**:
```qml
property var accountInfo: null        // 账户信息对象
property bool ctpConnected: true      // CTP 连接状态
property string accountId: "SIM-207611"  // 账户 ID
```

### 2. MarketQuotePanel.qml - 实时行情面板
**位置**: `/home/zd/A-Trader/qt_manager/qml/MarketQuotePanel.qml`

**功能**:
- 显示订阅的合约行情列表（合约代码、最新价、成交量）
- 支持通过输入框快速订阅新合约
- 左键点击行情项选中合约并自动填入价格
- 右键菜单支持：
  - 📌 置顶：将合约移到列表顶部
  - ⬇️ 置底：将合约移到列表底部
  - ❌ 取消订阅：取消订阅并从列表移除
- 空白区域右键菜单：
  - ➕ 新增合约：打开对话框添加新合约

**对外属性**:
```qml
property var marketModel: null        // 行情数据模型
property var orderController: null    // 订单控制器
property var zmqWorker: null          // ZMQ 通信工作器
```

### 3. PositionPanel.qml - 持仓面板
**位置**: `/home/zd/A-Trader/qt_manager/qml/PositionPanel.qml`

**功能**:
- 显示当前持仓列表
- 显示持仓信息：合约、方向、数量、盈亏、成本/现价
- 点击持仓自动选中合约并订阅行情
- 自动填入最新价到下单面板
- 盈亏自动变色（红涨绿跌）

**对外属性**:
```qml
property var positionModel: null      // 持仓数据模型
property var orderController: null    // 订单控制器
```

### 4. OrderPanel.qml - 下单面板
**位置**: `/home/zd/A-Trader/qt_manager/qml/OrderPanel.qml`

**功能**:
- 显示当前选中的合约
- 价格输入（支持自动跟价和手动输入）
- 数量输入（支持 +/- 按钮和直接输入）
- 显示预估保证金和手续费
- 买开、卖开、平仓按钮

**对外属性**:
```qml
property var orderController: null    // 订单控制器
```

## main.qml 简化效果

### 重构前
- 总行数: **657 行**
- 账户面板: 50 行
- 行情部分: 320 行
- 持仓部分: 51 行
- 下单面板: 190 行

### 重构后
- 总行数: **75 行**（减少 582 行，约 **88.6%**）
- 账户面板: 6 行（使用 AccountInfoPanel 组件）
- 行情部分: 8 行（使用 MarketQuotePanel 组件）
- 持仓部分: 8 行（使用 PositionPanel 组件）
- 下单面板: 5 行（使用 OrderPanel 组件）

### 代码对比

**重构前（657 行）**:
```qml
ApplicationWindow {
    // 50 行账户面板代码...
    // 320 行行情列表代码...
    // 51 行持仓列表代码...
    // 190 行下单面板代码...
}
```

**重构后（75 行）**:
```qml
ApplicationWindow {
    ColumnLayout {
        AccountInfoPanel { ... }      // 6 行
        
        RowLayout {
            ColumnLayout {
                MarketQuotePanel { ... }  // 8 行
                PositionPanel { ... }     // 8 行
            }
            OrderPanel { ... }            // 5 行
        }
    }
}
```

## 优势

1. **可维护性提升** ⬆️ - 每个组件职责单一，修改互不影响
2. **可读性提升** ⬆️ - main.qml 结构清晰，一目了然
3. **可复用性** ⬆️ - 组件可在其他页面复用
4. **易于测试** ✅ - 可单独测试每个组件
5. **团队协作** 👥 - 可并行开发不同组件
6. **代码量减少** 📉 - 从 657 行减少到 75 行，减少 **88.6%**

## 文件结构

```
qt_manager/qml/
├── main.qml                    # 主窗口（75 行，仅布局）✨
├── AccountInfoPanel.qml        # 账户信息面板组件 ✨ 新增
├── MarketQuotePanel.qml        # 实时行情面板组件 ✨ 新增
├── PositionPanel.qml           # 持仓面板组件 ✨ 新增
├── OrderPanel.qml              # 下单面板组件 ✨ 新增
├── qml.qrc                     # 资源文件（已更新）
└── README_REFACTOR.md          # 重构说明文档
```

## 组件化完成情况

| 组件 | 状态 | 原始行数 | 组件化后 | 减少比例 |
|------|------|----------|----------|----------|
| 账户信息面板 | ✅ 完成 | 50 行 | 6 行 | 88% |
| 实时行情面板 | ✅ 完成 | 320 行 | 8 行 | 97.5% |
| 持仓面板 | ✅ 完成 | 51 行 | 8 行 | 84.3% |
| 下单面板 | ✅ 完成 | 190 行 | 5 行 | 97.4% |
| **总计** | **✅ 完成** | **657 行** | **75 行** | **88.6%** |

## 后续扩展建议

如果需要添加更多功能，可以考虑创建以下组件：
1. **OrderHistoryPanel.qml** - 报单历史面板（显示历史委托记录）
2. **TradeHistoryPanel.qml** - 成交历史面板（显示历史成交记录）
3. **ChartPanel.qml** - K线图表面板（集成图表库显示行情走势）
4. **StrategyPanel.qml** - 策略管理面板（管理和监控交易策略）

通过这种组件化架构，可以轻松扩展新功能而不影响现有代码！
