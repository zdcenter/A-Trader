# 音频文件说明

此目录应包含以下音频文件：

1. `condition_triggered.wav` - 条件单触发时的提示音
   - 建议：较高音调的"叮"声或成功提示音
   
2. `condition_cancelled.wav` - 条件单取消时的提示音
   - 建议：较低音调的"咚"声或警告提示音

## 如何添加音频文件

您可以：
1. 使用在线音频生成工具（如 https://www.zapsplat.com/ 或 https://freesound.org/）
2. 使用音频编辑软件（如 Audacity）创建简单的提示音
3. 从系统声音库中复制合适的音频文件

## 临时方案

如果暂时没有音频文件，可以：
1. 从 `/usr/share/sounds/` 目录复制系统提示音
2. 使用以下命令生成简单的音频文件（需要安装 sox）：
   ```bash
   # 触发音（高音）
   sox -n condition_triggered.wav synth 0.2 sine 800
   
   # 取消音（低音）
   sox -n condition_cancelled.wav synth 0.2 sine 400
   ```
