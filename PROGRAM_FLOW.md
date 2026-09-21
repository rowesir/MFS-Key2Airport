# Key2Airport 程序流程

> 版本：v3.1
> 最后更新：2026-09-22
> 说明：本文档按讨论逐步补充，后续控件逻辑逐节追加。代码实现以本文档为准。

---

## 1. 程序启动初始化

启动时做两件事：初始化主窗口 `Widget`，以及初始化作为其值成员的 `DialogEnum`（枚举/监听窗口）。此时模拟器可以还没启动，界面处于待机状态。

程序入口（`main.cpp`）另外做了两件事：把 `QT_QPA_PLATFORM` 设成 `windows:darkmode=0`（关掉 Windows 深色模式），以及把界面样式设成 `Fusion`。界面外观依赖这两项。

### 1.1 主窗口 Widget

| 控件 | 类型 | 初始状态 | 实现要点 |
| --- | --- | --- | --- |
| `cbConfig` | QComboBox | 空、失能 | 清空所有项；`setEnabled(false)` |
| `ckbAuto` | QCheckBox | 已勾选 | `setChecked(true)` |
| `lbInfo` | QLabel | `Standby...` | 初始文本；之后由连接流程改写（`Waiting MFS...` / `Connected` / `Aircraft Loaded` / `Sim Error`），见第 4、5 节 |
| `pbtnEnum` | QPushButton | 失能 | `setEnabled(false)`。期望是"连接成功后使能"，但**目前没有任何代码打开它**，待实现 |
| `ckbRA` | QCheckBox | 已勾选 | `setChecked(true)` |
| `ckbLR` | QCheckBox | 已勾选 | `setChecked(true)` |
| `lcdRA` | QLCDNumber | 显示 `----` | `display("----")` |
| `lcdLR` | QLCDNumber | 显示 `-----` | `display("-----")` |
| `lbPage` | QLabel | 空 | 清空文本 |
| `lbDetail` | QLabel | 空 | 清空文本；设置 `Qt::TextSelectableByMouse`，用户可选中文本复制 |

LCD 初始显示：用减号做占位，位数与各自的 `digitCount` 一致 —— `lcdRA` 是 4 位，显示 `----`；`lcdLR` 是 5 位，显示 `-----`。已核对 Qt 源码 `qlcdnumber.cpp`：`-`（minus）在段码表里是有效符号，且属于类文档列出的可显示字符，只有非法字符才会被替换成空格，所以这个方法可用。

配套的 `.ui` 调整：

- `cbConfig` 里现有的示例项 `Cessna 172 Cessna Cessna` 应删除，避免 Qt Designer 预览与运行时不一致。
- `lcdRA` / `lcdLR` 上的 `value` / `intValue` 属性（2500 / -1234）应删除，理由同上。

尚未定义初始状态的控件：`pbtnFolder`（待补充）。`pbtnConnect` 的状态机见第 4 节。

### 1.2 DialogEnum（枚举 / 监听窗口）

生命周期：`DialogEnum` 是 `Widget` 的**值成员**（不是指针），随 `Widget` 构造而构造、随其析构而析构。窗口可反复开关，因此界面初始化必须可以被重复调用。

控件名沿用现有 `.ui` 中的拼写（`TWListem`）。

初始化集中在一个函数里：

```cpp
class DialogEnum : public QDialog
{
    Q_OBJECT

public:
    explicit DialogEnum(QWidget *parent = nullptr);
    ~DialogEnum();

    void initUI();          // 建列 + 清表，可重复调用

private:
    Ui::DialogEnum *ui;
};
```

调用时机：

1. 构造函数末尾调用一次 —— 因此程序启动时两张表已经是空的，满足"程序开始时清空"的要求；
2. 每次打开窗口前再调用一次 —— 因此每次打开窗口都是干净的两张表。

当前打开方式：**没有任何入口**。早期临时把枚举窗口挂在 `pbtnConnect` 上当调试入口，后来 `pbtnConnect` 改成了连接状态机（第 4 节），这个入口就没了；`pbtnEnum` 是失能的、也没有槽函数。等开始做枚举流程时再把入口接上（大概率是 `pbtnEnum`，连接成功后使能、点击后先 `initUI()` 再 `exec()`）。

实现约定：`on_pbtnConnect_clicked` 必须声明在类的 `private slots:` 里。`setupUi` 的自动连接是 `QMetaObject::connectSlotsByName`，它只遍历元对象里注册过的方法，普通成员函数不会被连接。

`initUI()` 的职责：

1. 给 `TWEnumAll` 建立 3 列；
2. 给 `TWListem` 建立 5 列；
3. 两张表统一设置：不可编辑、不排序、禁止拖动列、列宽按窗口宽度平均分配（`QHeaderView::Stretch`）；
4. 清空两张表的所有行（`setRowCount(0)`）。

建列部分重复调用无害（会被覆盖），所以构造函数与打开前调用同一个函数即可。

不属于 `initUI()` 的内容：hash → param 这类**纯数据缓存**不随界面清空，否则每次开关窗口都要重新查询参数签名。

#### 1.2.1 TWEnumAll 列定义

用途：显示枚举出的当前飞机全部控件，每条对应一个 `SIMCONNECT_INPUT_EVENT_DESCRIPTOR`。

| 列 | 表头 | 数据来源 | 显示形式 |
| --- | --- | --- | --- |
| 0 | Name | `SIMCONNECT_INPUT_EVENT_DESCRIPTOR::Name`（char[64]） | 原样字符串 |
| 1 | Hash | `::Hash`（UINT64） | 十进制字符串 |
| 2 | eType | `::eType` | `DOUBLE` / `STRING` |

3 列，与结构体三个字段一一对应。

#### 1.2.2 TWListem 列定义

用途：显示监听期间收到的每一条 `SIMCONNECT_RECV_SUBSCRIBE_INPUT_EVENT`。

| 列 | 表头 | 数据来源 | 填充时机 |
| --- | --- | --- | --- |
| 0 | Hash | 结构体 `::Hash`（UINT64） | 收到通知时立即填充 |
| 1 | eType | 结构体 `::eType` | 收到通知时立即填充 |
| 2 | Value | 结构体 `::Value`（长度由 `eType` 决定） | 收到通知时立即填充 |
| 3 | Param | `SimConnect_EnumerateInputEventParams` 返回的签名字符串，如 `;FLOAT64` | 回调返回后补填 |
| 4 | Size | 由 Param 换算出的字节数，即将来 `SetInputEvent` 的 `cbUnitSize`（如 `;FLOAT64` → 8） | 回调返回后补填 |

5 列。

Param / Size 是异步的：`EnumerateInputEventParams` 是"发请求 → 等回调"，因此一行的插入分两阶段完成 —— 先插入 Hash / eType / Value 三列，回调到了再补 Param / Size。参数按 hash 缓存，同一个 hash 只有第一条触发查询，后面的行直接取缓存填充。

已知边界情况：签名返回空串的事件（例如部分除冰开关），Size 会算成 0，但它实际仍需要发一个 double。这属于后续发送逻辑要处理的问题，不在初始化范围内。

#### 1.2.3 两张表的通用行为

- **不可编辑**：`setEditTriggers(QAbstractItemView::NoEditTriggers)`。用户双击、按 F2 都无法修改单元格内容。
- **可复制**：Ctrl+C 复制光标所在的那一格。这是 Qt item view 的内置行为，不需要额外写代码。
- **不排序**：不启用 `setSortingEnabled`，点击表头不排序。
- **列宽自适应**：所有列用 `QHeaderView::Stretch`，按窗口宽度平均分配，并随窗口缩放。不用 `ResizeToContents` —— 表初始化时是空的，按内容算宽度会让每列只剩表头那么宽，全部挤在左边。
- **禁止拖动列**：表头不允许拖动换位。

### 1.3 DialogEnum 对外接口

外部（`Widget`）通过下面这些接口把数据送进两张表：

```cpp
// TWEnumAll：追加到末尾
void addEnumAll(const QString &name, quint64 hash, const QString &eType);

// TWListem：插入到第 0 行
void addListen(quint64 hash, const QString &eType, const QString &value,
               const QString &param, int size);
```

共两个接口，一次把一行写完。

插入方向：

- `TWEnumAll` 往**尾部**添加（`insertRow(rowCount())`），枚举结果按到达顺序往下排。
- `TWListem` 往**头部**添加（`insertRow(0)`），最新一条永远在最上面。同一个 hash 反复变化就产生多行：不去重、不合并、不更新已有行。

Param / Size 的来源：调用方按 hash 缓存查询（同一个 hash 只查一次），查到后随通知一起传给 `addListen`。参数用空串、size 用 -1 表示"本次还没查到"，该格留空；之后同一个 hash 的行都会有值。

参数类型：接口只收 `name / hash / eType / value / param / size` 这些普通字段，不收 SimConnect 结构体，这样 `DialogEnum` 不必依赖 SimConnect 头文件。

---

## 2. 输入监听

初始化完成后立即开始监听，无开关、无按钮。

### 2.1 总体要求

- 监听范围：本机**所有键盘**、**所有游戏控制器**，以及**鼠标的滚轮和侧键**（鼠标左右键、中键不监听）。
- 纯监听：不能影响原有输入行为，按键照常传给系统和游戏。
- 程序最小化、失去焦点、被其他窗口盖住时，监听必须照常工作。这是核心要求。

### 2.2 线程模型

- **输入线程**：键盘钩子、DirectInput 轮询、设备重枚举。只干监听，不放别的东西。
- **SDK 线程**：SimConnect 的连接、派发，以后的事件枚举与发送都放这里。
- **主线程**：只接收两个线程发来的信号、做界面显示。
- 为什么不把 SDK 和输入放一个线程：模拟器加载航班时 SimConnect 调用可能阻塞数秒，会把按键检测一起拖住（键盘钩子被拖住还可能被系统摘掉）。用定时器也解决不了，定时器只是换个时机阻塞，仍在同一个线程上。
- 输入线程将来要发送事件时，只把请求用信号投递给 SDK 线程，自己不等结果。
- **跨线程调用必须排队**：给另一个线程里的对象发请求，要用信号槽或 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`，不能直接调它的方法。直接调会在**调用者**线程里执行，函数里创建的子对象（`QTimer`、`QWinEventNotifier` 之类）会挂到错误的线程上——Qt 会报 `Cannot create children for a parent that is in a different thread`，而且那些对象之后会在错误的线程上工作（踩过一次：SDK 的重试和派发实际跑到了主线程）。

### 2.3 键盘

- 方案：`SetWindowsHookEx(WH_KEYBOARD_LL, ...)` 低级键盘钩子。系统级生效，不依赖窗口焦点，后台可用。
- 被动：回调里无条件调用 `CallNextHookEx` 并把返回值原样返回，绝不返回非 0 去拦截。
- 回调里只做一件事：把按键交给界面层。绝不做耗时操作（Windows 对低级钩子有超时限制，超时会被系统静默摘掉钩子）。
- 程序退出时必须 `UnhookWindowsHookEx`。
- 按键标识用虚拟键码（VK）。
- 需要记录修饰键的按下/抬起状态，用于组合键显示。
- 键名来自固定的 VK → 名称表（常用键），未收录的用 `GetKeyNameText` 兜底。

### 2.4 lbDetail 显示规则

- 键盘显示按键本身的名字，不加前缀：按 B 显示 `B`，按 CTRL 显示 `CTRL`。
- 游戏控制器必须带设备名，否则不同外设的同号按键会显示成一个：例如 `T.16000M Button 3`、`TCA Q-Eng 1&2 Button 3`。lbDetail 上的字符串就是后续绑定的依据，不能有歧义。
- 支持组合键：按住修饰键再按其他键，显示成 `L CTRL+B` —— 修饰键在前、普通键在后，用 `+` 连接。
- 修饰键只包括键盘的 CTRL / SHIFT / ALT / WIN，**区分左右**：左 Ctrl 显示 `L CTRL`，右 Ctrl 显示 `R CTRL`，其余同理（`L SHIFT` / `R SHIFT` / `L ALT` / `R ALT` / `L WIN` / `R WIN`）。
- 游戏控制器按键是普通按键，不作为修饰键。
- 显示至少 10 秒：按下时启动 10 秒定时器，到点自动清空。留这么长是为了让用户有时间选中文本复制（复制出来的字符串就是后续配置表要用的绑定依据）。
- 10 秒内又有新按键按下：立即改显示新键，并重新计时。
- 长按产生的重复 keydown 只刷新计时，不改变显示内容。
- 不改变字体。

### 2.5 游戏控制器（DirectInput8）

- 只监听按键和 POV 帽，不监听轴。
- 显示必须带设备身份：`设备名 + 控件名`，例如 `T.16000M Button 3`、`T.16000M POV 0 Up`。因为不同设备的"Button 3"完全不同，而 lbDetail 上的字符串就是绑定依据，不区分就会出现"两个外设的按键触发同一个操作"。
- 设备名取 DirectInput 的 `DIDEVICEINSTANCE.tszInstanceName`（本机是 `T.16000M`、`TCA Q-Eng 1&2`）。如果同时接了两台同名设备，追加序号区分：`T.16000M #1 Button 3`、`T.16000M #2 Button 3`。
- 内部表示同样带设备身份 —— 不能用设备路径（换一个 USB 口路径就变），要用 VID/PID 加产品名。内部键按"来源 : 设备 : 控件"三段统一（**设计约定，目前还没落到代码里**，现在只有显示字符串）：

| 来源 | deviceKey | controlKey | 完整键示例 |
| --- | --- | --- | --- |
| 键盘 | `kbd` | VK 名 | `kbd:VK_B` |
| DirectInput | 产品标识（VID/PID） | 按钮序号 | `di:044F-B10A:BTN_3` |

通道已定：**DirectInput8**。因为实际接入的是图马斯特 T16000M 摇杆 + TCA 空客油门，这类设备 XInput 看不到，必须走 DirectInput。

已实测（本机接 T16000M + TCA Q-Eng 1&2）：

- 两台设备都能被 `EnumDevices(DI8DEVCLASS_GAMECTRL, ..., DIEDFL_ATTACHEDONLY)` 枚举到，`SetDataFormat(&c_dfDIJoystick2)` + `DISCL_BACKGROUND | DISCL_NONEXCLUSIVE` + `Acquire` + `GetDeviceState` 全部返回成功，也就是**后台非独占可读**。
- 设备名：`T.16000M`、`TCA Q-Eng 1&2`。
- **按钮名不用 DirectInput 返回的名字**：它是本地化的（本机返回 `按钮 0`、`按钮 1`……）。统一生成英文 `Button 0`、`Button 1`……，因为后续绑定以英文为准。
- **坑**：`EnumObjects` 返回的 `dwOfs` 不是 `DIJOYSTATE2` 里的下标（实测 T16000M 是 24..39、TCA 是 40..70），不能拿来索引。实测状态数据本身是标准 `DIJOYSTATE2` 布局（POV 在字节 32、静止值 `0xFFFFFFFF`，轴在 0..23），所以按钮要按**枚举顺序**索引 `rgbButtons[0..N-1]`。

通用性：按标准游戏控制器类别（`DI8DEVCLASS_GAMECTRL`）枚举，摇杆、手柄、油门、方向盘、脚舵都在内，没有任何针对特定型号的代码。上限是 `DIJOYSTATE2` 格式的 128 个按钮；不以游戏控制器身份出现的纯 HID 按钮盒不在这一类（若它模拟键盘输入，则由键盘钩子覆盖）。

热插拔：**不做定时重枚举**。`EnumDevices` 一次要 170 ms（实测），定时跑纯属浪费且会拖住同线程的键盘钩子。改成主窗口收到 `WM_DEVICECHANGE` 时通知工作线程重枚举，并且 500 ms 防抖（设备插拔会连着来好几个广播）。启动时枚举一次。

只监听不拦截：协作级别用 `DISCL_BACKGROUND | DISCL_NONEXCLUSIVE`，设备是共享的，本程序只调用 `GetDeviceState` 读取自己那一份数据，从不独占获取、不写设备属性、不设置力反馈，所以游戏那边的输入完全不受影响。

POV 帽：枚举 `DIDFT_POV` 对象，按序号读 `rgdwPOV[i]`，把 1/100 度的角度量化成 8 个方向（0=Up、4500=Up-Right、9000=Right、13500=Down-Right、18000=Down、22500=Down-Left、27000=Left、31500=Up-Left），居中值 `0xFFFFFFFF` 视为松开。方向发生变化时发一次事件，命名 `POV 0 Up`、`POV 0 Up-Right` 这样（带序号是因为可能有多个帽）。4 向帽不会报对角线值，同一套量化逻辑直接通用。实测 T16000M 报 1 个 POV（`帽状开关`，usage 0x39），TCA 油门没有 POV。

坑：`EnumObjects` 的过滤参数必须写成 `DIDFT_BUTTON | DIDFT_POV`。只传 `DIDFT_BUTTON` 时 POV 对象不会进回调，看上去就像"这个设备没有帽"。

### 2.6 鼠标滚轮与侧键

- 只监听四样：滚轮上、滚轮下、侧键 1（后退）、侧键 2（前进）。左右键和中键不监听。
- 显示名：`Mouse Wheel Up`、`Mouse Wheel Down`、`Mouse Side 1`、`Mouse Side 2`。
- 通道：DirectInput 鼠标（`GUID_SysMouse` + `c_dfDIMouse2` + `DISCL_BACKGROUND | DISCL_NONEXCLUSIVE`），复用现有的 10 ms 轮询，不需要枚举设备。
- 不用鼠标低级钩子（`WH_MOUSE_LL`）的原因：它会把全系统鼠标输入串到本程序的线程上，而工作线程在设备插拔时会卡 170 ms（实测），那样会造成全机鼠标卡顿。键盘能承受这个代价，鼠标不行。
- 不用 RawInput 的原因：RawInput 会把每一次鼠标移动都作为 `WM_INPUT` 送进来（高刷新率鼠标每秒上千次），白白增加主线程负载。DirectInput 轮询只在需要时读状态，没有这个问题。
- 滚轮读的是**相对量**：每次读取拿到的是"上次读取之后累计的滚动量"，正值向上。所以一次轮询最多发一次事件，连续滚动会合并成一次，不会刷出一串重复。
- 支持组合键：滚轮或侧键事件发生时，先查一遍修饰键的**物理状态**（`GetAsyncKeyState`），按住哪个就加哪个前缀，例如 `L CTRL+Mouse Wheel Up`、`R SHIFT+Mouse Side 1`。修饰键集合和顺序与 2.4 一致（CTRL / SHIFT / ALT / WIN，区分左右）。用 `GetAsyncKeyState` 而不是和键盘监听器共享状态，是为了让两个监听器互不依赖——鼠标在事件发生的那一刻直接问系统要真实状态，最省事也最稳。
- 已知风险：如果游戏独占鼠标，可能读不到，需要实机确认；真读不到的话退路是 RawInput（`RIDEV_INPUTSINK`）或鼠标低级钩子。

轮询频率：实测 Qt 定时器在 Windows 上无需调整系统计时精度即可跑准 —— 16 ms 实测 16.0 ms、4 ms 实测 4.0 ms、1 ms（配合 `timeBeginPeriod(1)`）实测 1.0 ms。另实测 `SetEventNotification` 在本机这些设备上返回 `0x800700AA`（ERROR_BUSY），事件驱动不可用，只能轮询。

当前取值 **10 ms（100 Hz）**。空闲 CPU 实测（单核占比，6 秒采样）：不轮询时约 1.6%、16 ms 约 2.6%、10 ms 约 2.3%、4 ms 约 3.4%。受采样精度限制，16 ms 与 10 ms 两个数字的差别在噪声范围内，4 ms 是唯一明显更高的一档。1 ms 需要 `timeBeginPeriod(1)`、会抬高全系统计时精度，不采用。

---

## 3. 控件互斥保护

规则：任何可点击控件的槽函数执行期间，主窗口里**所有**可点击控件都处于失能状态，包括触发它的那个；槽函数退出时按进入前的状态还原。

- 还原成"进入前的状态"，不是一律恢复可用。进入前就是失能的（例如启动时 `cbConfig`、`pbtnEnum` 本来就是失能），退出后仍然失能。
- 槽函数执行期间被改动过使能状态的控件不再还原，保留改动结果（例如连接成功后把 `pbtnEnum` 打开，退出后它就是可用的）。
- 范围：主窗口内的 `QAbstractButton` 和 `QComboBox`，即 `pbtnConnect`、`pbtnEnum`、`pbtnFolder`、`cbConfig`、`ckbAuto`、`ckbRA`、`ckbLR`。实现上遍历子控件自动收集，以后新增控件自动生效。`lbInfo`、`lbDetail`、`lbPage`、`lcdRA`、`lcdLR` 不算（不可点击）；`DialogEnum` 里的控件不算（独立窗口，打开时是模态，主窗口点不到）。
- 用法：每个槽函数第一行写 `ControlGuard guard(this);`，构造时快照并失能，析构时还原，所以中途 `return` 也会正确恢复。
- 槽函数之间不会互相调用，因此不需要嵌套计数。
- 已知边界：槽函数"主动保持失能"（控件本来可用、执行期间要求它保持失能）检测不到——Qt 的 `setEnabled` 在状态没变化时不产生任何事件。真需要这种语义得在槽里显式声明。

---

## 4. 连接按钮（pbtnConnect）

用成员变量 `connected` 判断状态，初始为 `false`（程序刚打开就是未连接）。

未连接状态下点击：

- `connected` 置为 `true`；
- `cbConfig`、`ckbAuto` 保持失能（不是只在槽函数执行期间失能，而是之后都不恢复）；
- `lbInfo` 显示 `Waiting MFS...`；
- 按钮文字变为 `Disconn`；
- 按钮图标变为 `:/Resoure/CoilRed.png`，图标尺寸不变（仍是 18x18）。

已连接状态下点击：

- `connected` 置回 `false`；
- `ckbAuto` 恢复可用（它是连接时被失能的，断开要还原；直接 `setEnabled(true)`，守卫会识别为"槽函数改过"从而保留）；
- `cbConfig` 保持失能（启动时它就是失能，连接时只是又被失能一次，断开后维持原状）；
- `lbInfo` 恢复 `Standby...`；
- 按钮文字恢复 `Connect`，图标恢复 `:/Resoure/CoilBalck.png`。

因此连一次再断一次，界面应回到与启动时完全一致的状态。

调用方式：连接与断开**不是直接调** `simClient` 的方法，而是通过 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 投递到 SDK 线程执行（原因见 2.2）。SDK 层本身见第 5 节。

实现注意：`cbConfig`、`ckbAuto` 的失能是用 `ControlGuard::keepDisabled()` 声明的，不能只在槽里调 `setEnabled(false)`。原因是槽函数执行期间守卫已经把这两个控件置为失能了，槽里再调一次是空操作、检测不到，守卫退出时会按进入前的状态把它们恢复回来（`ckbAuto` 本来可用，就会被错误地恢复成可用）。`keepDisabled()` 就是明确告诉守卫"这个控件退出时保持失能"。

---

## 5. SimConnect 客户端

类 `SimConnectClient`，跑在**自己的 SDK 线程**里（与输入线程分开，见 2.2）。以后的枚举、飞机识别、事件发送都放这个线程。

连接：

- `SimConnect_Open(&h, "Key2Airport", nullptr, 0, hEvent, SIMCONNECT_OPEN_CONFIGINDEX_LOCAL)`
- 不传窗口句柄，改用**事件句柄**：连接成功后模拟器会在这个事件上发信号，SDK 线程用 `QWinEventNotifier` 接住，再调 `SimConnect_CallDispatch` 派发。这样整条链路都在 SDK 线程里，不依赖主线程的窗口消息。
- `ConfigIndex` 用 `SIMCONNECT_OPEN_CONFIGINDEX_LOCAL`，强制连本机，避免被遗留的 `SimConnect.cfg` 指到别的机器上去。
- 模拟器没启动时 `SimConnect_Open` 直接返回失败，用 1 秒的定时器重试直到成功。这段等待期界面就停在 `Waiting MFS...`。
- 模拟器退出时收到 `SIMCONNECT_RECV_ID_QUIT`：关闭当前句柄但**保持重试**，自动重连（详见 5.1）。
- 手动断开：`SimConnect_Close` + 关闭事件句柄，界面复位回 `Standby...`。

部署：

- exe 运行时必须能加载 `SimConnect.dll`，否则进程直接起不来。
- `.pro` 里加了 `QMAKE_POST_LINK`，链接完成后自动把 SDK 里的 `SimConnect.dll` 拷到产物目录（`$(DESTDIR)`）。

### 5.1 状态通知

四个状态都有对应的机制：

| 状态 | SimConnect 机制 | 本工程的信号 |
| --- | --- | --- |
| 已连接 | `SimConnect_Open` 成功 | `connected()` |
| 已加载飞机 | `SimConnect_SubscribeToSystemEvent(..., "AircraftLoaded")`，回包是 `SIMCONNECT_RECV_ID_EVENT_FILENAME`，带 `szFileName` | `aircraftLoaded(QString)` |
| 模拟停止（退出飞行的瞬间） | `Sim` 的 `dwData = 0`，回包是 `SIMCONNECT_RECV_ID_EVENT` | `flightEnded()` |
| 异常 / 错误退出 | `SIMCONNECT_RECV_ID_EXCEPTION`（`dwException` 是错误码）；连接被结束是 `SIMCONNECT_RECV_ID_QUIT` | `simError(quint32)` / `disconnected()` |

连接成功后会立刻订阅 `AircraftLoaded` 和 `Sim`。订阅 `Sim` 时系统会**立刻回一次当前状态**。"连不上"这个状态不需要单独通知，界面停在 `Waiting MFS...` 就是在重试。

**实测结论（2026-09-22，事件探针）**：主菜单自己也在跑一个模拟，并且会加载一次预览飞机。典型序列：

```
SimStop=0, Sim=0                     ← 模拟停止
FlightLoaded  ...\CustomFlight\CustomFlight.FLT
SimStart=0, Sim=1                    ← 菜单里的预览模拟启动了
AircraftLoaded ...\asobo_cj4\...\aircraft.cfg     ← 预览飞机加载
```

由此可知：

- **`Sim` 不能用来判断"在飞行中"**——退出飞行 0.8 秒后它会因为菜单预览又变回 1；
- **`AircraftLoaded` 在主菜单也会发**（刚进游戏、还没进正式飞行就会发一次）。

当前实现是最简单的一版，尚未处理上述两点：

- `AircraftLoaded` → 界面 `Aircraft Loaded`；
- `Sim` 回 0 → 界面 `Connected`。

**所以主菜单（以及刚进游戏时）也会显示 `Aircraft Loaded`——这是已知问题，见第 7 节。** `SimStart` / `SimStop` 也在飞行边界触发（实测进入飞行时先 `SimStop=0` 再 `SimStart=0`），但成对出现且方向不直观，暂时不用它们做判据。

重连策略：

| 情况 | 行为 |
| --- | --- |
| `SimConnect_Open` 返回 `E_FAIL`（模拟器没运行） | 正常，1 秒后继续重试，界面停在 `Waiting MFS...` |
| `SimConnect_Open` 返回其他错误（异常情况） | 停止重试，界面复位回 `Standby...` 并显示错误 |
| `SIMCONNECT_RECV_ID_QUIT`（模拟器退出、连接断开） | **自动重连**：关掉当前句柄但保持重试，界面停在 `Waiting MFS...`（按钮仍是 `Disconn`），模拟器重新起来后自动连上，并自动重新订阅飞机 / 飞行事件 |
| `SIMCONNECT_RECV_ID_EXCEPTION` | 只把错误码报给界面，**不**断开连接——单次调用报错不代表连接坏了 |
| 用户手点 `Disconn` | 停止重试，界面复位回 `Standby...` |

只有两种情况会停止重试：用户手动断开，以及 `SimConnect_Open` 返回非 `E_FAIL` 的异常错误。

还没接、但以后可能用得上的系统事件：`SimStart` / `SimStop`（已在探针里观察过，成对触发）、`Pause` / `Pause_EX1`（暂停 / 恢复，`dwData` 含义未整理）。

界面文案**待定**，现在都是占位：`connected` → `Connected`、`aircraftLoaded` → `Aircraft Loaded`、`simError` → `Sim Error`。

`aircraftLoaded` 信号里带的是 `szFileName` 原始字符串（飞机配置文件的路径）。要显示成机型号而不是路径的话，得先看这个字符串实际长什么样，或者改成读 `TITLE` / `ATC MODEL` 这类 SimVar。

重试间隔现在固定 1 秒，也可以调。

---

## 6. 待补充

- `pbtnFolder` 的行为（初始状态、点击后做什么）
- `pbtnEnum` 的槽函数：现在既失能又没有槽；枚举窗口目前**没有任何入口能打开**
- 枚举流程（把 `EnumerateInputEvents` 的结果填进 `TWEnumAll`）与监听流程（把订阅通知填进 `TWListem`）
- 飞机识别：现在只有 `AircraftLoaded` 的文件路径，要显示机型 / 做按机型匹配，得读 `TITLE`、`ATC MODEL` 这类 SimVar
- 事件发送流程（按键 → 匹配 → `SetInputEvent`）
- 配置文件的读写与按机型切换
- 界面文案定稿：`Connected` / `Aircraft Loaded` / `Sim Error` 目前都是占位

---

## 7. 已知问题（待解决）

### 7.1 主菜单被当成"飞机已加载"

- **症状**：进了游戏还停在主菜单、没进正式飞行，`lbInfo` 就显示 `Aircraft Loaded`；结束飞行退回主菜单后也会保持 `Aircraft Loaded`。
- **原因**：主菜单自身也在跑模拟并加载预览飞机，因此会发 `AircraftLoaded`（探针日志里 `87.89s` 那条就是菜单预览，当时前面并没有任何 `FlightLoaded`）。
- **当前状态**：**未解决**。代码现在是最简单规则（收到 `AircraftLoaded` 就显示），所以主菜单也会显示。
- **为什么先不猜着修**：能区分"菜单预览加载"和"真实飞行加载"的判据还没确定。日志里那条 `FlightLoaded(...\CustomFlight\CustomFlight.FLT)` 到底是"开始自由飞行"还是"回到主菜单"尚未确认——两种解释会推出**相反**的判断逻辑，猜错就会把"进入飞行显示 `Aircraft Loaded`"这个已经好用的行为弄坏（2026-09-22 曾按"回菜单"实现过一版，因无法确认而撤回）。
- **下一步**：用事件探针（`probe_events.exe`）分阶段抓一次日志——主菜单停 30 秒 → 进入飞行停 30 秒 → 退回主菜单停 30 秒 → 退出。阶段边界清楚之后，就能一次性确定 `FlightLoaded` 的含义以及 `Sim` / `SimStart` / `SimStop` / `Pause` 在三个阶段里的表现，再据此定规则。

---

## 8. 变更记录

| 日期 | 版本 | 变更 |
| --- | --- | --- |
| 2026-09-21 | v0.1 | 初稿：程序启动初始化、两张表的列定义、TWListem 行插入规则 |
| 2026-09-21 | v0.2 | lcdRA / lcdLR 初始显示改为 `----`；明确复制为单元格级、无需额外代码；`label_3` 移出待补充 |
| 2026-09-21 | v0.3 | 新增 1.3 DialogEnum 对外接口：TWEnumAll 尾部追加、TWListem 头部插入、Param/Size 句柄补填 |
| 2026-09-21 | v0.4 | 接口简化为两个：`addListen` 一次写入 5 列，去掉句柄与补填接口 |
| 2026-09-21 | v0.5 | `lbDetail` 加入初始化清单；`lcdLR` 占位符改为 5 位；记录 `pbtnConnect` 的临时调试入口与 slot 声明约定 |
| 2026-09-21 | v0.6 | 列宽模式由 `ResizeToContents` 改为 `Stretch`，随窗口宽度自动分配 |
| 2026-09-21 | v0.7 | 新增第 2 节输入监听：总体要求、键盘钩子、lbDetail 显示规则、游戏控制器表示法建议 |
| 2026-09-21 | v0.8 | lbDetail 改为显示具体按键名并支持组合键；控制器只监听按键、不加显示前缀 |
| 2026-09-21 | v0.9 | 修饰键区分左右（`L CTRL`）；控制器通道定为 DirectInput8，记录实际设备 |
| 2026-09-21 | v1.0 | DirectInput 实测结论：设备枚举/后台读取成功、按钮名照用、dwOfs 不可作索引、轮询与重枚举策略 |
| 2026-09-21 | v1.1 | 按钮名改英文 `Button N`；补充通用性、只监听不拦截、轮询频率实测（4 ms） |
| 2026-09-21 | v1.2 | 轮询频率由 4 ms 调整为 10 ms（100 Hz），补充各档空闲 CPU 实测 |
| 2026-09-21 | v1.3 | 增加 POV 帽支持：8 方向量化、`POV N 方向` 命名；轴仍不监听 |
| 2026-09-21 | v1.4 | 修正 `EnumObjects` 过滤参数（`DIDFT_BUTTON \| DIDFT_POV`），否则 POV 对象不会被枚举 |
| 2026-09-21 | v1.5 | 监听整体移入工作线程（主线程只收信号显示）；取消定时重枚举，改为 `WM_DEVICECHANGE` 触发 + 500 ms 防抖 |
| 2026-09-21 | v1.6 | 新增第 3 节控件互斥保护：槽函数执行期间失能全部可点击控件，退出时按进入前状态还原 |
| 2026-09-21 | v1.7 | 新增第 4 节连接按钮状态；补充 `ControlGuard::keepDisabled()` 及其必要性 |
| 2026-09-21 | v1.8 | 控制器显示改为"设备名 + 控件名"，同名设备追加 `#1`/`#2`，避免两台外设的同号按键显示成一个 |
| 2026-09-21 | v1.9 | lbDetail 显示时长 1 秒改为 10 秒；lbDetail 设为可选中复制 |
| 2026-09-21 | v2.0 | 新增 2.6 鼠标滚轮与侧键（DirectInput 鼠标轮询）；2.1 监听范围同步更新 |
| 2026-09-21 | v2.1 | 鼠标滚轮/侧键支持修饰键组合（`L CTRL+Mouse Wheel Up` 这类） |
| 2026-09-21 | v2.2 | 补全 pbtnConnect 的断开分支：状态复位、`ckbAuto` 恢复可用、文案与图标还原 |
| 2026-09-21 | v2.3 | 新增第 5 节 SimConnect 客户端：连接/重试/退出检测/断开、事件句柄派发、DLL 部署 |
| 2026-09-21 | v2.5 | 第 5 节补充 5.1 状态通知：已连接 / 已加载飞机 / 异常三种状态的 SimConnect 机制与对应信号 |
| 2026-09-21 | v2.6 | 2.2 线程模型改为双工作线程；补充"跨线程调用必须排队"的规则 |
| 2026-09-21 | v2.7 | 5.1 补充重连策略：模拟器退出后自动重连，只有手动断开或异常错误才停止 |
| 2026-09-21 | v2.8 | 5.1 增加"退出飞行"状态：订阅系统事件 `Sim`，回 0 时界面从 `Aircraft Loaded` 回到 `Connected` |
| 2026-09-22 | v2.9 | 与代码逐条核对并修正：枚举窗口入口已失效、`lbInfo`/`pbtnEnum` 初始状态说明、SDK 独立线程、自动重连、`Sim` 已接入、待补充清单重写 |
| 2026-09-22 | v3.0 | 用事件探针实测更正"退出飞行"判定：菜单自身也会跑模拟并重发 `AircraftLoaded`，改用 `FlightLoaded`(CustomFlight) 识别菜单、`Sim`=0 清除菜单模式；信号 `simRunningChanged` 改为 `flightEnded` |
| 2026-09-22 | v3.1 | 撤回 v3.0 里基于 `FlightLoaded` 的菜单判据（依据未确认，猜反会破坏"进入飞行"）；新增第 7 节记录未解决的"主菜单被当成飞机已加载"问题与验证方案 |
