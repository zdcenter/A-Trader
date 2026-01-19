# 实时行情面板 - 完整功能说明

## ✅ 已实现的完整功能

### 1. 丰富的数据列 📊

支持 **12 个数据列**，可自由配置显示：

| 列名 | 说明 | 默认显示 | 数据来源 |
|------|------|----------|----------|
| 合约 | 合约代码 | ✅ | CTP 行情 |
| 最新价 | 最新成交价 | ✅ | CTP 行情 |
| 涨跌 | 涨跌额 | ✅ | 计算值 |
| 幅度% | 涨跌幅百分比 | ✅ | 计算值 |
| 成交量 | 当日成交量 | ✅ | CTP 行情 |
| 持仓量 | 当前持仓量 | ❌ | CTP 行情 |
| 买一价 | 买一档价格 | ✅ | CTP 行情 |
| 买一量 | 买一档数量 | ❌ | CTP 行情 |
| 卖一价 | 卖一档价格 | ✅ | CTP 行情 |
| 卖一量 | 卖一档数量 | ❌ | CTP 行情 |
| 昨收 | 昨日收盘价 | ❌ | CTP 行情 |
| 时间 | 最后更新时间 | ❌ | CTP 行情 |

### 7. 架构改进与问题修复 (2026-01-18)

### 7.1 组件化架构优化
为了解决组件化过程中的数据绑定和生命周期问题，进行了以下关键改进：

1.  **Context Property 命名空间隔离**
    - C++ 端注入的属性现在带有 `App` 前缀（如 `AppMarketModel`），避免与 QML 组件内部属性名（如 `marketModel`）发生冲突。
    - 彻底解决了 "Binding loop detected" 警告。

2.  **对象生命周期管理**
    - 核心业务对象（Model, Controller）现在在堆上创建（`new`），并指定 `QApplication` 为父对象。
    - 防止了局部变量销毁导致的潜在崩溃问题。

3.  **嵌套模型访问修复**
    - 在 `MarketQuotePanel` 的 `ListView` Delegate 中，显式将 `model` 捕获为 `rowData` 属性。
    - 解决了 `Repeater` 内部无法正确访问外层数据模型（导致 `undefined` 错误）的问题。

### 7.2 已知问题解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| **Binding loop** | 组件属性名与全局 Context Property 同名，QML 引擎无法区分 | C++ 端重命名 Context Property (`App...`)，实现命名空间隔离 |
| **TypeError: undefined** | `Repeater` 的 `model` 遮蔽了外层 `ListView` 的 `model` | 在 Delegate 根部添加 `property var rowData: model` 显式保存引用 |
| **Settings 警告** | 未设置 `OrganizationName` | 在 `main.cpp` 中调用 `setOrganizationName` 等函数 |
| **无合约显示** | 动态列宽计算在初始化时 `availableWidth` 为 0 | 添加保护逻辑，宽度未就绪时使用默认值 |

### 7.3 CTP Core 协议升级 (2026-01-18)
为了支持更丰富的深度行情展示，Core 端的消息协议已升级，新增了以下字段：
- `pre_close`, `pre_settlement` (昨收/昨结)
- `upper_limit`, `lower_limit` (涨跌停)
- `open`, `high`, `low`, `close` (开高低收)
- `turnover` (成交额), `avg_price` (均价)

QT Manager 端已同步更新 `MarketModel` 以解析这些新字段，从而能够正确计算和显示涨跌幅。

### 7.4 合约拖拽排序 (2026-01-18)
支持在行情列表中长按/拖拽合约行进行排序。
- **操作**：鼠标左键按住合约行并拖动，即可调整顺序。
- **持久化**：排序结果会自动保存，下次启动时会按照保存的顺序加载合约列表。
- **恢复逻辑**：启动时会先按顺序创建占位行，再发起订阅，确保顺序不被打乱。

**操作方式**：
- 点击行情面板右上角的 ⚙️ 按钮
- 在弹出菜单中勾选/取消勾选列
- ✅ **菜单保持打开**：可以连续选择多个列
- 点击菜单外部或按 ESC 键关闭菜单

**特性**：
- ✅ 支持显示/隐藏任意列（除"合约"列固定显示）
- ✅ 配置立即生效
- ✅ 配置自动保存到本地
- ✅ 下次打开自动恢复配置

### 3. 列拖拽排序 🔄

**操作方式**：
- 直接按住表头的列标题
- 左右拖动到目标位置
- 释放鼠标完成排序

**特性**：
- ✅ 拖拽时有视觉反馈（半透明影子）
- ✅ 数据行实时跟随表头列顺序变化
- ✅ 排序结果自动保存

### 4. 智能布局 📐

**特性**：
- ✅ **隐藏列不占位**：取消勾选的列完全不占用空间
- ✅ **自动填充**：显示的列会自动分配可用空间
- ✅ **横向滚动**：如果列太多，支持横向滚动查看

### 5. 配置持久化 💾

**保存内容**：
1. **列配置**：
   - 列的显示/隐藏状态
   - 列的顺序
   - 列的宽度

2. **订阅列表**：
   - 所有已订阅的合约代码
   - 合约在列表中的顺序

**保存时机**：
- ✅ 切换列可见性时立即保存
- ✅ 拖拽列排序后立即保存
- ✅ 程序关闭时保存订阅列表

**恢复时机**：
- ✅ 程序启动后 1 秒自动恢复列配置
- ✅ 程序启动后 1 秒自动重新订阅上次的合约

**存储位置**：
- Linux: `~/.config/qt_manager/qt_manager.conf`
- Windows: `%APPDATA%/qt_manager/qt_manager.ini`

### 6. 颜色高亮 🎨

**涨跌颜色**：
- 🔴 红色：上涨（正值）
- 🟢 绿色：下跌（负值）
- ⚪ 白色：平盘

**应用范围**：
- 涨跌额
- 涨跌幅
- 最新价（根据涨跌判断）
- 买一价/卖一价（根据涨跌判断）

## 🎯 使用指南

### 场景 1：自定义显示列

1. 点击 ⚙️ 按钮打开列配置菜单
2. 勾选"持仓量"和"时间"
3. 取消勾选"买一量"和"卖一量"
4. 继续勾选/取消其他列（菜单保持打开）
5. 点击菜单外部或按 ESC 关闭
6. 配置立即生效并自动保存
7. 下次打开程序，配置自动恢复

### 场景 2：调整列顺序

1. 按住"涨跌幅"列标题
2. 拖动到"合约"列后面
3. 释放鼠标
4. 列顺序变更，配置自动保存
5. 下次打开程序，顺序保持不变

### 场景 3：订阅管理

1. 在输入框输入合约代码（如 rb2505）并回车订阅
2. 继续订阅其他合约：p2505, m2505
3. 右键点击合约可以置顶/置底或取消订阅
4. 关闭程序
5. 重新打开程序
6. ✅ 自动重新订阅这 3 个合约，顺序保持一致

## 🔧 技术细节

### 菜单保持打开的实现

```qml
Menu {
    id: columnConfigMenu
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    
    MenuItem {
        checkable: true
        onTriggered: {
            // 更新配置
            columnModel.setProperty(index, "visible", checked)
            saveColumnConfig()
        }
        // 不调用 menu.close()，菜单保持打开
    }
}
```

### 隐藏列不占位的实现

```qml
delegate: Rectangle {
    visible: model.visible
    width: model.visible ? model.width : 0  // 隐藏时宽度为 0
    // ...
}
```

### 配置持久化的实现

```qml
Settings {
    id: marketSettings
    category: "MarketQuotePanel"
    property string columnConfig: ""
    property string subscribedInstruments: ""
}

// 保存
function saveColumnConfig() {
    marketSettings.columnConfig = JSON.stringify(config)
}

// 恢复
Component.onCompleted: {
    var savedConfig = JSON.parse(marketSettings.columnConfig)
    // 重建 columnModel
}
```

## 📊 配置文件示例

`~/.config/qt_manager/qt_manager.conf`:

```ini
[MarketQuotePanel]
columnConfig=[{"role":"instrumentId","title":"合约","width":80,"visible":true},{"role":"lastPrice","title":"最新价","width":80,"visible":true}...]
subscribedInstruments=["rb2505","p2505","m2505"]
```

## 🚀 未来扩展建议

可以考虑添加：
1. **列宽调整**：拖动列边界调整宽度（保存到配置）
2. **排序功能**：点击列标题按该列排序（升序/降序）
3. **过滤功能**：根据条件过滤显示的合约
4. **导出功能**：导出当前行情数据到 CSV
5. **自选分组**：将合约分组管理（如：黑色系、化工等）
6. **预设方案**：保存多套列配置方案，一键切换
7. **涨跌排行**：按涨跌幅排序，快速找到强势/弱势品种

## 📝 注意事项

1. **首次使用**：第一次打开时使用默认配置
2. **配置冲突**：如果配置文件损坏，会自动回退到默认配置
3. **订阅恢复**：需要确保 CTP Core 已启动，否则订阅会失败
4. **性能考虑**：订阅过多合约可能影响性能，建议控制在 20 个以内
5. **菜单操作**：点击菜单外部或按 ESC 键关闭配置菜单

## ✨ 改进点总结

### 最新改进（2026-01-18）

1. ✅ **菜单保持打开**
   - 之前：点击一次菜单项就关闭
   - 现在：可以连续选择多个列，点击外部才关闭

2. ✅ **隐藏列不占位**
   - 之前：隐藏的列仍然占用空间
   - 现在：隐藏的列宽度为 0，完全不占位

3. ✅ **自动填充空间**
   - 之前：列宽固定，可能有空白
   - 现在：显示的列自动分配可用空间

4. ✅ **完整持久化**
   - 列配置自动保存和恢复
   - 订阅列表自动保存和恢复
   - 程序重启后完全恢复上次状态
