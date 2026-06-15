# ESP32-S3 舵机控制器 - 调试指南

## 📦 所需材料

| 物品 | 数量 | 备注 |
|------|------|------|
| ESP32-S3 开发板 | 1 块 | 如 Xiao ESP32S3、ESP32-S3-DevKitC |
| SG90 舵机 | 1 个 | 9g 微型舵机 |
| 杜邦线 | 若干 | 公对母或母对母 |
| 5V 电源 (可选) | 1 个 | 如果舵机抖动建议外接 |

## 🔧 接线步骤

### 1. SG90 舵机线序识别
- **棕色线**: GND (接地)
- **红色线**: VCC (电源正极 4.8-6V)
- **橙色线**: Signal (信号线)

### 2. ESP32-S3 连接

**推荐接线方案：**
```
SG90 棕色线 → ESP32-S3 GND
SG90 红色线 → ESP32-S3 5V (或 VIN)
SG90 橙色线 → ESP32-S3 GPIO40
```

**备选 PWM 引脚** (如果 GPIO40 被占用)：
- GPIO39
- GPIO41
- GPIO42
- GPIO45

> ⚠️ **注意**：ESP32-S3 的部分 GPIO 有特殊用途，避免使用 GPIO0 (BOOT 键)、GPIO1 (TX)、GPIO2 (RX)

### 3. 接线检查清单
- [ ] 确认 GND 共地
- [ ] 确认 VCC 接 5V 引脚 (不要接 3.3V)
- [ ] 确认信号线接 GPIO40 (或备选引脚)
- [ ] 确认接线牢固无松动

## 💻 程序烧录

### 方法一：Arduino IDE

1. **安装 ESP32 开发板支持**
   - 打开 Arduino IDE
   - 文件 → 首选项 → 附加开发板管理器网址
   - 添加：`https://espressif.github.io/arduino-esp32/package_esp32_dev_index.json`
   - 工具 → 开发板 → 开发板管理器
   - 搜索 "esp32" 并安装 **"esp32 by Espressif Systems"** (版本 2.0.9 或更高)

2. **选择 ESP32-S3 开发板**
   - 工具 → 开发板 → ESP32 Arduino → **Xiao ESP32S3** (或你使用的具体型号)
   - 常见选项：
     - `Xiao ESP32S3` (Seeed Studio)
     - `ESP32S3 Dev Module` (通用)
     - `Adafruit Feather ESP32-S3`
   - 工具 → Port → 选择对应的 COM 端口

3. **安装 Servo 库**
   - 项目 → 加载库 → 管理库
   - 搜索 **"ESP32Servo"** 并安装 by Kevin Harrington
   - ⚠️ **不要使用标准 Servo 库**，ESP32-S3 需要专用库

4. **修改 WiFi 配置**
   打开 `servo_controller.ino`，修改以下两行：
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";        // 改成你的 WiFi 名称
   const char* password = "YOUR_WIFI_PASSWORD"; // 改成你的 WiFi 密码
   ```

5. **上传程序**
   - **首次上传可能需要进入下载模式**：
     - 按住 BOOT 键
     - 点击上传按钮 (→)
     - 松开 BOOT 键
   - 等待编译和上传完成 (S3 编译时间稍长)
   - 打开串口监视器 (波特率 115200)
   - 按 RESET 键运行程序

### 方法二：PlatformIO (VS Code)

1. 安装 PlatformIO 扩展
2. 创建新项目，选择 **ESP32-S3**
3. 在 `platformio.ini` 添加：
   ```ini
   [env:esp32-s3]
   platform = espressif32
   board = esp32-s3-devkitc-1
   framework = arduino
   lib_deps = 
       ESP32Servo
   build_flags = 
       -D CORE_DEBUG_LEVEL=3
   ```
4. 将代码复制到 `src/main.cpp`
5. 修改 WiFi 配置
6. 点击上传

## 🔍 调试步骤

### 步骤 1: 串口监视器检查

上传程序后，打开串口监视器 (115200 波特率)，应该看到：

```
ESP32-S3 舵机控制器启动
正在连接 WiFi...
WiFi 连接成功！
IP 地址：192.168.1.100
信号强度 (RSSI): -65 dBm
ESP32-S3 MAC 地址：XX:XX:XX:XX:XX:XX
mDNS 启动：http://esp32servo.local
HTTP 服务器已启动
=================================
```

**常见问题：**

| 现象 | 可能原因 | 解决方案 |
|------|----------|----------|
| 一直显示"正在连接 WiFi..." | WiFi 密码错误或信号弱 | 检查密码，靠近路由器 |
| 显示"WiFi 连接失败" | SSID 或密码错误 | 仔细检查配置 |
| 舵机抖动或不转 | 供电不足或引脚错误 | 外接 5V 电源，检查 GPIO40 |
| 编译错误"Servo.h not found" | 未安装 ESP32Servo 库 | 安装 ESP32Servo 库 |
| 上传失败 | 未进入下载模式 | 按住 BOOT 键再按 RESET |
| 无任何输出 | 串口波特率错误或上传失败 | 确认 115200，重新上传 |

### 步骤 2: Web 界面访问

1. **通过 IP 访问**
   - 在浏览器输入串口显示的 IP 地址
   - 例如：`http://192.168.1.100`

2. **通过 mDNS 访问** (如果支持)
   - 在浏览器输入：`http://esp32servo.local`
   - 注意：部分 Windows 系统需要安装 Bonjour 服务

3. **手机访问**
   - 确保手机和 ESP32 在同一 WiFi
   - 用手机浏览器访问 IP 地址

### 步骤 3: 舵机测试

1. **基础测试**
   - 打开 Web 界面
   - 拖动角度滑块
   - 观察舵机是否平滑转动
   - 检查 Web 界面的舵机臂动画是否同步

2. **速度测试**
   - 点击不同速度按钮 (慢/中/快/最快)
   - 拖动角度滑块
   - 观察舵机转动速度变化

3. **预设位置测试**
   - 点击 0°、90°、180° 按钮
   - 确认舵机准确到达指定位置

## ⚠️ ESP32-S3 特有问题与解决方案

### 问题 1: 编译失败或警告

**原因**: ESP32-S3 需要特定版本的库

**解决方案**:
```cpp
// 确保使用 ESP32Servo 库，不是标准 Servo
#include <ESP32Servo.h>  // ✅ 正确
// #include <Servo.h>    // ❌ 错误

// 如果编译仍有问题，在 setup() 添加：
myServo.setPeriodHertz(50);
myServo.attach(SERVO_PIN, 500, 2400);
```

### 问题 2: USB 识别问题

**原因**: ESP32-S3 有原生 USB，但驱动可能未安装

**解决方案**:
- **Windows**: 安装 CH340/CP2102 驱动 (根据开发板芯片)
- **Mac**: 通常无需驱动
- **Linux**: 可能需要添加 udev 规则

### 问题 3: 进入下载模式困难

**原因**: ESP32-S3 需要手动进入下载模式

**解决方案**:
```
方法 1 (推荐):
1. 按住 BOOT 键
2. 点击上传按钮
3. 等待出现"Connecting..."
4. 松开 BOOT 键

方法 2:
1. 按住 BOOT 键
2. 按一下 RESET 键
3. 松开 BOOT 键
4. 点击上传按钮
```

### 问题 4: 舵机抖动严重

**原因**: 供电不足 (S3 峰值电流更大)

**解决方案**:
```
方案 A: 使用外接 5V 电源 (强烈推荐)
- 外接 5V 电源正极 → SG90 红色线
- 外接 5V 电源负极 → ESP32-S3 GND (共地)
- ESP32-S3 5V 引脚不接

方案 B: 使用高质量 USB 线
- 使用短而粗的 USB 线
- 使用 2A 以上的 USB 充电器
- 避免使用延长线
```

### 问题 5: PWM 信号异常

**原因**: 引脚选择不当

**解决方案**:
```cpp
// 更换 PWM 兼容引脚
#define SERVO_PIN 40  // 默认推荐
// #define SERVO_PIN 39  // 备选 1
// #define SERVO_PIN 41  // 备选 2
// #define SERVO_PIN 42  // 备选 3

// 调整 PWM 脉宽范围
myServo.attach(SERVO_PIN, 500, 2400); // 标准 SG90
// myServo.attach(SERVO_PIN, 600, 2300); // 如果角度不够
```

### 问题 6: WiFi 连接不稳定

**原因**: ESP32-S3 的 WiFi 天线设计差异

**解决方案**:
1. 检查 PCB 天线区域是否有金属遮挡
2. 使用外置天线 (如果开发板支持)
3. 靠近路由器测试
4. 添加 WiFi 重连代码：
```cpp
// 在 loop() 中添加：
if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi 断开，重新连接...");
    connectToWiFi();
    delay(5000);
}
```

## 🎯 ESP32-S3 性能优化

### 1. 利用双核优势
```cpp
// ESP32-S3 是双核，可以并行处理
void loop() {
    server.handleClient();  // 核心 1
    moveServoSmoothly();    // 核心 2
    MDNS.update();
}
```

### 2. 调整 PWM 精度
```cpp
// ESP32-S3 支持更高分辨率
// 在 setup() 中添加：
ledcSetup(0, 50, 10);  // 50Hz, 10 位分辨率
ledcAttachPin(SERVO_PIN, 0);
```

### 3. 降低功耗 (电池供电时)
```cpp
// 在空闲时降低 CPU 频率
void loop() {
    if (!isMoving) {
        setCpuFrequencyMhz(80);  // 降到 80MHz
    } else {
        setCpuFrequencyMhz(240); // 恢复 240MHz
    }
    // ... 其他代码
}
```

## 📊 ESP32-S3 vs ESP32-C3 对比

| 特性 | ESP32-S3 | ESP32-C3 |
|------|----------|----------|
| CPU | 双核 240MHz | 单核 160MHz |
| SRAM | 512KB | 400KB |
| GPIO | 45 个 | 22 个 |
| USB | 原生 OTG | 无 |
| AI 指令集 | ✅ 支持 | ❌ 不支持 |
| 功耗 | 稍高 | 更低 |
| 价格 | 稍贵 | 更便宜 |
| 舵机控制 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |

## 🎉 ESP32-S3 扩展建议

1. **USB 键盘/鼠标模拟**
   - 利用 S3 的 USB OTG 功能
   - 制作 HID 设备

2. **摄像头集成**
   - 连接 OV2640/OV5640
   - 制作 WiFi 云台相机

3. **显示屏驱动**
   - 连接 SPI 屏幕
   - 本地显示角度信息

4. **语音控制**
   - 添加麦克风模块
   - 语音识别控制舵机

5. **多舵机云台**
   - S3 引脚多，可控制 4-8 个舵机
   - 制作相机云台或机械臂

## 📱 使用技巧

1. **快速定位**: 直接点击 0°/90°/180° 按钮快速归位
2. **精细调节**: 拖动滑块进行微调
3. **速度选择**: 
   - 慢速 (1-2): 精细操作
   - 中速 (3-5): 日常使用
   - 快速 (6-8): 快速响应
   - 最快速 (9-10): 演示效果
4. **MAC 地址记录**: 用于设备识别和网络管理

## 🔐 安全注意事项

1. **不要长时间堵转**: 舵机到达限位后不要持续发送同角度信号
2. **避免频繁快速转动**: 会缩短舵机寿命
3. **注意供电**: 确保电源稳定，S3 峰值电流更大
4. **散热**: 长时间使用时注意 ESP32-S3 和舵机的温度
5. **USB 保护**: 避免热插拔损坏 USB 端口

## 📊 技术参数 (ESP32-S3 版)

| 参数 | 数值 |
|------|------|
| 舵机角度范围 | 0-180° |
| 速度档位 | 1-10 可调 |
| WiFi 频率 | 2.4GHz 802.11 b/g/n |
| 工作电压 | 5V |
| PWM 频率 | 50Hz |
| PWM 分辨率 | 10 位 |
| 控制距离 | 室内约 30-50 米 |
| CPU 频率 | 240MHz (双核) |

---

**ESP32-S3 版本完成！性能更强，扩展性更好~** 🚀

有任何问题随时问我哦！(๑•̀ㅂ•́)و✧
