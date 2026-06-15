/*
 * ESP32-S3 WiFi 舵机控制器 (SG90S 数字舵机版)
 * 功能：通过 Web 界面远程控制 SG90S 舵机的角度和速度
 * 作者：Loomy
 * 日期：2026-06-16
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>

// ==================== 配置区域 ====================
const char* ssid = "Slyyy-2.4G";        // 替换为你的 WiFi 名称
const char* password = "Sly101rua"; // 替换为你的 WiFi 密码

// 引脚定义 (ESP32-S3)
#define SERVO_PIN 40  // S3 的 GPIO40 是推荐的 PWM 引脚

// SG90S 数字舵机参数
#define MIN_PULSE_WIDTH 500   // SG90S 最小脉宽 (μs)
#define MAX_PULSE_WIDTH 2500  // SG90S 最大脉宽 (μs)

// 360 度舵机说明
// 90°: 停止
// 90°-180°: 正转 (角度越大速度越快)
// 0°-90°: 反转 (角度越小速度越快)

// ==================== 全局变量 ====================
WebServer server(80);
Servo myServo;

// 360 度舵机参数
int servoSpeed = 0;       // 速度 (-100 到 100, 正数正转，负数反转，0 停止)
int targetSpeed = 0;      // 目标速度
bool isMoving = false;

unsigned long lastMoveTime = 0;
unsigned long speedUpdateInterval = 50; // 速度更新间隔 (ms)

// ==================== HTML 页面 ====================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>ESP32-S3 舵机控制器 (360 度连续旋转)</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        
        .container {
            background: white;
            border-radius: 20px;
            padding: 40px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 500px;
            width: 100%;
        }
        
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 10px;
            font-size: 24px;
        }
        
        .status {
            text-align: center;
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }
        
        .status.connected {
            color: #27ae60;
        }
        
        .control-section {
            margin-bottom: 30px;
        }
        
        .label {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 10px;
            font-weight: 600;
            color: #555;
        }
        
        .value-display {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 5px 15px;
            border-radius: 20px;
            font-size: 16px;
            min-width: 60px;
            text-align: center;
        }
        
        input[type="range"] {
            width: 100%;
            height: 8px;
            border-radius: 5px;
            background: #e0e0e0;
            outline: none;
            -webkit-appearance: none;
        }
        
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 24px;
            height: 24px;
            border-radius: 50%;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            cursor: pointer;
            box-shadow: 0 2px 10px rgba(102, 126, 234, 0.4);
        }
        
        .speed-control {
            display: flex;
            gap: 10px;
            margin-top: 10px;
        }
        
        .speed-btn {
            flex: 1;
            padding: 10px;
            border: 2px solid #667eea;
            background: white;
            color: #667eea;
            border-radius: 10px;
            cursor: pointer;
            font-weight: 600;
            transition: all 0.3s;
        }
        
        .speed-btn.active {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border-color: transparent;
        }
        
        .speed-btn:hover {
            background: #667eea;
            color: white;
        }
        
        .servo-visual {
            background: #f5f5f5;
            border-radius: 15px;
            padding: 30px;
            margin: 30px 0;
            position: relative;
            height: 150px;
            display: flex;
            justify-content: center;
            align-items: flex-end;
        }
        
        .servo-arm {
            width: 10px;
            height: 100px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            border-radius: 5px;
            position: relative;
            transform-origin: bottom center;
            transform: rotate(0deg);
            transition: transform 0.1s;
        }
        
        @keyframes spin {
            from { transform: rotate(0deg); }
            to { transform: rotate(360deg); }
        }
        
        .servo-base {
            width: 60px;
            height: 60px;
            background: #333;
            border-radius: 50%;
            position: absolute;
            bottom: 20px;
        }
        
        .angle-markers {
            position: absolute;
            top: 10px;
            left: 0;
            right: 0;
            display: flex;
            justify-content: space-between;
            padding: 0 20px;
            color: #999;
            font-size: 12px;
        }
        
        .btn-group {
            display: flex;
            gap: 10px;
            margin-top: 20px;
        }
        
        .btn {
            flex: 1;
            padding: 15px;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s;
        }
        
        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        
        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 20px rgba(102, 126, 234, 0.4);
        }
        
        .btn-secondary {
            background: #f0f0f0;
            color: #333;
        }
        
        .btn-secondary:hover {
            background: #e0e0e0;
        }
        
        .info-panel {
            background: #f8f9fa;
            border-radius: 10px;
            padding: 15px;
            margin-top: 20px;
            font-size: 13px;
            color: #666;
        }
        
        .info-panel p {
            margin-bottom: 5px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🤖 360° 舵机控制器</h1>
        <p class="status connected" id="statusText">● 已连接</p>
        
        <div class="servo-visual">
            <div class="angle-markers">
                <span>反转</span>
                <span>停止</span>
                <span>正转</span>
            </div>
            <div class="servo-arm" id="servoArm"></div>
            <div class="servo-base"></div>
        </div>
        
        <div class="control-section">
            <div class="label">
                <span>旋转速度</span>
                <span class="value-display" id="speedValue">0</span>
            </div>
            <input type="range" id="speedSlider" min="-100" max="100" value="0">
            <div style="display: flex; justify-content: space-between; margin-top: 5px; color: #999; font-size: 12px;">
                <span>← 全速反转</span>
                <span>停止</span>
                <span>全速正转 →</span>
            </div>
        </div>
        
        <div class="control-section">
            <div class="label">
                <span>速度档位</span>
                <span class="value-display" id="gearValue">中速</span>
            </div>
            <div class="speed-control">
                <button class="speed-btn" data-speed="-100">反转</button>
                <button class="speed-btn active" data-speed="0">停止</button>
                <button class="speed-btn" data-speed="50">慢速</button>
                <button class="speed-btn" data-speed="100">快速</button>
            </div>
        </div>
        
        <div class="btn-group">
            <button class="btn btn-secondary" onclick="setSpeed(-100)">← 反转</button>
            <button class="btn btn-secondary" onclick="setSpeed(0)">⏹ 停止</button>
            <button class="btn btn-secondary" onclick="setSpeed(100)">正转 →</button>
        </div>
        
        <div class="info-panel">
            <p>📡 WiFi 信号：<span id="wifiSignal">--</span></p>
            <p>🔧 当前状态：<span id="servoStatus">停止</span></p>
            <p>💡 提示：360 度舵机连续旋转，无法定位角度</p>
        </div>
    </div>
    
    <script>
        const speedSlider = document.getElementById('speedSlider');
        const speedValue = document.getElementById('speedValue');
        const gearValue = document.getElementById('gearValue');
        const servoArm = document.getElementById('servoArm');
        const servoStatus = document.getElementById('servoStatus');
        const wifiSignal = document.getElementById('wifiSignal');
        const speedBtns = document.querySelectorAll('.speed-btn');
        
        let currentSpeed = 0;
        let rotationAngle = 0;
        
        // 速度滑块事件
        speedSlider.addEventListener('input', function() {
            const speed = parseInt(this.value);
            speedValue.textContent = speed;
            
            // 更新滑块颜色
            if (speed < 0) {
                this.style.background = `linear-gradient(to right, #e74c3c 0%, #e74c3c ${50 + speed/2}%, #e0e0e0 ${50 + speed/2}%, #e0e0e0 100%)`;
            } else if (speed > 0) {
                this.style.background = `linear-gradient(to right, #e0e0e0 0%, #e0e0e0 ${50}%, #667eea ${50}%, #667eea ${50 + speed/2}%, #e0e0e0 ${50 + speed/2}%)`;
            } else {
                this.style.background = '#e0e0e0';
            }
            
            // 动画显示
            updateServoAnimation(speed);
        });
        
        speedSlider.addEventListener('change', function() {
            const speed = parseInt(this.value);
            updateServo(speed);
        });
        
        // 速度按钮事件
        speedBtns.forEach(btn => {
            btn.addEventListener('click', function() {
                speedBtns.forEach(b => b.classList.remove('active'));
                this.classList.add('active');
                currentSpeed = parseInt(this.dataset.speed);
                speedSlider.value = currentSpeed;
                speedValue.textContent = currentSpeed;
                
                // 更新档位显示
                if (currentSpeed === 0) gearValue.textContent = '停止';
                else if (currentSpeed < 0) gearValue.textContent = '反转';
                else if (currentSpeed <= 50) gearValue.textContent = '慢速';
                else gearValue.textContent = '快速';
                
                updateServo(currentSpeed);
                updateServoAnimation(currentSpeed);
            });
        });
        
        // 预设速度按钮
        function setSpeed(speed) {
            speedSlider.value = speed;
            speedValue.textContent = speed;
            
            if (speed === 0) gearValue.textContent = '停止';
            else if (speed < 0) gearValue.textContent = '反转';
            else if (speed <= 50) gearValue.textContent = '慢速';
            else gearValue.textContent = '快速';
            
            speedBtns.forEach(btn => {
                btn.classList.toggle('active', parseInt(btn.dataset.speed) === speed);
            });
            
            updateServo(speed);
            updateServoAnimation(speed);
        }
        
        // 更新舵机
        function updateServo(speed) {
            if (speed === 0) {
                servoStatus.textContent = '停止';
            } else if (speed < 0) {
                servoStatus.textContent = `反转 (${Math.abs(speed)}%)`;
            } else {
                servoStatus.textContent = `正转 (${speed}%)`;
            }
            
            fetch(`/servo?speed=${speed}`)
                .then(response => response.json())
                .then(data => {
                    console.log('舵机状态:', data);
                });
        }
        
        // 更新动画
        function updateServoAnimation(speed) {
            if (speed === 0) {
                servoArm.style.animation = 'none';
                servoArm.style.transform = `rotate(${rotationAngle - 90}deg)`;
            } else {
                const duration = Math.abs(2000 / speed); // 速度越快，旋转周期越短
                servoArm.style.animation = `spin ${duration}s linear ${speed > 0 ? '' : 'reverse'} infinite`;
            }
        }
        
        // 定时获取状态
        function getStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    speedSlider.value = data.speed;
                    speedValue.textContent = data.speed;
                    wifiSignal.textContent = data.rssi + ' dBm';
                    
                    if (data.speed === 0) {
                        gearValue.textContent = '停止';
                        servoStatus.textContent = '停止';
                    } else if (data.speed < 0) {
                        gearValue.textContent = '反转';
                        servoStatus.textContent = `反转 (${Math.abs(data.speed)}%)`;
                    } else if (data.speed <= 50) {
                        gearValue.textContent = '慢速';
                        servoStatus.textContent = `正转 (${data.speed}%)`;
                    } else {
                        gearValue.textContent = '快速';
                        servoStatus.textContent = `正转 (${data.speed}%)`;
                    }
                    
                    speedBtns.forEach(btn => {
                        btn.classList.toggle('active', parseInt(btn.dataset.speed) === data.speed);
                    });
                    
                    updateServoAnimation(data.speed);
                })
                .catch(err => console.error('获取状态失败:', err));
        }
        
        // 每 2 秒更新一次状态
        setInterval(getStatus, 2000);
        getStatus();
    </script>
</body>
</html>
)rawliteral";

// ==================== 函数声明 ====================
void handleRoot();
void handleServo();
void handleSpeed();
void handleStatus();
void handleNotFound();
void moveServoSmoothly();
void connectToWiFi();

// ==================== 设置函数 ====================
void setup() {
    Serial.begin(115200);
    Serial.println("\n\nESP32-S3 舵机控制器启动 (360 度连续旋转)");
    
    // 初始化 360 度舵机 (ESP32-S3)
    myServo.setPeriodHertz(50);    // 50Hz 标准舵机频率
    myServo.attach(SERVO_PIN, 500, 2500); // 脉宽范围 500-2500us
    myServo.write(90); // 90 度停止
    delay(500);
    
    // 连接 WiFi
    connectToWiFi();
    
    // 配置 mDNS
    if (MDNS.begin("esp32servo")) {
        Serial.println("mDNS 启动：http://esp32servo.local");
    }
    
    // 配置 Web 服务器路由
    server.on("/", handleRoot);
    server.on("/servo", handleServo);
    server.on("/speed", handleSpeed);
    server.on("/status", handleStatus);
    server.onNotFound(handleNotFound);
    
    // 启动服务器
    server.begin();
    Serial.println("HTTP 服务器已启动");
    Serial.println("=================================");
}

// ==================== 主循环 ====================
void loop() {
    server.handleClient();
    moveServoSmoothly();
    // MDNS.update() 在新版 ESP32 库中已不需要
}

// ==================== 函数实现 ====================
void connectToWiFi() {
    Serial.print("正在连接 WiFi");
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi 连接成功！");
        Serial.print("IP 地址：");
        Serial.println(WiFi.localIP());
        Serial.print("信号强度 (RSSI): ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        Serial.print("ESP32-S3 MAC 地址：");
        Serial.println(WiFi.macAddress());
    } else {
        Serial.println("\nWiFi 连接失败，请检查配置");
    }
}

void handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

void handleServo() {
    if (server.hasArg("speed")) {
        int speed = server.arg("speed").toInt();
        
        // 限制速度范围 (-100 到 100)
        speed = constrain(speed, -100, 100);
        
        targetSpeed = speed;
        isMoving = (speed != 0);
        
        // 360 度舵机速度映射
        // 0: 停止，1-100: 正转速度，-1--100: 反转速度
        int servoAngle;
        if (speed == 0) {
            servoAngle = 90;  // 中位停止
        } else if (speed > 0) {
            // 正转：90° 到 180° (速度越快，角度越大)
            servoAngle = map(speed, 0, 100, 90, 170);
        } else {
            // 反转：90° 到 0° (速度越快，角度越小)
            servoAngle = map(abs(speed), 0, 100, 90, 10);
        }
        
        myServo.write(servoAngle);
        
        Serial.print("目标速度：");
        Serial.print(speed);
        Serial.print(" 映射角度：");
        Serial.println(servoAngle);
        
        // 返回 JSON 响应
        String jsonResponse = "{\"speed\":" + String(speed) + ",\"angle\":" + String(servoAngle) + ",\"status\":\"ok\"}";
        server.send(200, "application/json", jsonResponse);
    } else {
        server.send(400, "application/json", "{\"error\":\"缺少参数\"}");
    }
}

void handleSpeed() {
    if (server.hasArg("value")) {
        int speed = server.arg("value").toInt();
        speed = constrain(speed, -100, 100);
        targetSpeed = speed;
        isMoving = (speed != 0);
        
        // 映射到舵机角度
        int servoAngle;
        if (speed == 0) {
            servoAngle = 90;
        } else if (speed > 0) {
            servoAngle = map(speed, 0, 100, 90, 170);
        } else {
            servoAngle = map(abs(speed), 0, 100, 90, 10);
        }
        
        myServo.write(servoAngle);
        
        Serial.print("速度更新：");
        Serial.println(speed);
        
        String jsonResponse = "{\"speed\":" + String(speed) + ",\"status\":\"ok\"}";
        server.send(200, "application/json", jsonResponse);
    } else {
        server.send(400, "application/json", "{\"error\":\"缺少参数\"}");
    }
}

void handleStatus() {
    String statusText = isMoving ? (targetSpeed > 0 ? "正转" : "反转") : "停止";
    String jsonResponse = "{\"speed\":" + String(targetSpeed) + 
                          ",\"rssi\":" + String(WiFi.RSSI()) + 
                          ",\"status\":\"" + statusText + "\"}";
    server.send(200, "application/json", jsonResponse);
}

void handleNotFound() {
    server.send(404, "text/plain", "404: 未找到");
}

void moveServoSmoothly() {
    // 360 度舵机不需要平滑移动，直接设置速度
    // 此函数保留但简化，用于未来扩展
}
