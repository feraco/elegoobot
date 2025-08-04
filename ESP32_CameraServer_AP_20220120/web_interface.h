/*
 * Web Interface HTML Content
 * This file contains the Blockly interface for robot control
 */

#ifndef _WEB_INTERFACE_H_
#define _WEB_INTERFACE_H_

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Robot Controller</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: Arial, sans-serif; background: #f0f2f5; }
        .header { background: #fff; padding: 1rem; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .title { font-size: 1.5rem; font-weight: bold; color: #333; }
        .main { max-width: 1200px; margin: 2rem auto; padding: 0 1rem; }
        .workspace { display: grid; grid-template-columns: 1fr 300px; gap: 1rem; }
        .panel { background: #fff; border-radius: 8px; padding: 1rem; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }
        .blockly-area { height: 500px; border: 1px solid #ddd; }
        .controls { display: flex; gap: 0.5rem; margin-top: 1rem; }
        .btn { padding: 0.5rem 1rem; border: none; border-radius: 4px; cursor: pointer; font-weight: 500; }
        .btn-primary { background: #007bff; color: white; }
        .btn-danger { background: #dc3545; color: white; }
        .btn:hover { opacity: 0.9; }
        .camera { width: 100%; max-width: 280px; height: 210px; border-radius: 4px; }
        .sensor-grid { display: grid; gap: 0.5rem; margin-top: 1rem; }
        .sensor-item { display: flex; justify-content: space-between; padding: 0.5rem; background: #f8f9fa; border-radius: 4px; }
        .control-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 0.5rem; margin-top: 1rem; }
        .control-btn { padding: 1rem; border: none; border-radius: 8px; font-size: 1rem; font-weight: bold; cursor: pointer; }
        .move-forward { background: #28a745; color: white; }
        .move-backward { background: #6c757d; color: white; }
        .turn-left { background: #ffc107; color: black; }
        .turn-right { background: #fd7e14; color: white; }
        .stop-btn { background: #dc3545; color: white; grid-column: span 2; }
        .led-controls { margin-top: 1rem; }
        .color-btn { width: 40px; height: 40px; border: 2px solid #ddd; border-radius: 50%; margin: 0.25rem; cursor: pointer; }
        @media (max-width: 768px) { .workspace { grid-template-columns: 1fr; } }
    </style>
</head>
<body>
    <div class="header">
        <h1 class="title">🤖 Robot Controller</h1>
    </div>
    <div class="main">
        <div class="workspace">
            <div class="panel">
                <h3>Manual Controls</h3>
                <div class="control-grid">
                    <button class="control-btn move-forward" onclick="moveForward()">↑ Forward</button>
                    <button class="control-btn move-backward" onclick="moveBackward()">↓ Backward</button>
                    <button class="control-btn turn-left" onclick="turnLeft()">← Left</button>
                    <button class="control-btn turn-right" onclick="turnRight()">→ Right</button>
                    <button class="control-btn stop-btn" onclick="stopRobot()">⏹ STOP</button>
                </div>
                
                <div class="led-controls">
                    <h4>LED Colors</h4>
                    <div style="display: flex; flex-wrap: wrap;">
                        <div class="color-btn" style="background: red;" onclick="setLED(255,0,0)"></div>
                        <div class="color-btn" style="background: green;" onclick="setLED(0,255,0)"></div>
                        <div class="color-btn" style="background: blue;" onclick="setLED(0,0,255)"></div>
                        <div class="color-btn" style="background: yellow;" onclick="setLED(255,255,0)"></div>
                        <div class="color-btn" style="background: purple;" onclick="setLED(255,0,255)"></div>
                        <div class="color-btn" style="background: cyan;" onclick="setLED(0,255,255)"></div>
                        <div class="color-btn" style="background: white;" onclick="setLED(255,255,255)"></div>
                        <div class="color-btn" style="background: black;" onclick="setLED(0,0,0)"></div>
                    </div>
                </div>

                <div style="margin-top: 1rem;">
                    <h4>Servo Control</h4>
                    <label>Horizontal: <input type="range" id="servoH" min="0" max="180" value="90" onchange="controlServo(1, this.value)"></label><br>
                    <label>Vertical: <input type="range" id="servoV" min="30" max="110" value="70" onchange="controlServo(2, this.value)"></label>
                </div>
            </div>
            
            <div>
                <div class="panel">
                    <h3>Camera Feed</h3>
                    <img id="camera" src="/stream" class="camera" alt="Camera" onerror="this.src='/capture'">
                    <div style="margin-top: 0.5rem;">
                        <button class="btn btn-primary" onclick="startCamera()">Start</button>
                        <button class="btn btn-danger" onclick="stopCamera()">Stop</button>
                    </div>
                </div>
                
                <div class="panel" style="margin-top: 1rem;">
                    <h3>Sensor Data</h3>
                    <div class="sensor-grid">
                        <div class="sensor-item">
                            <span>Distance:</span>
                            <span id="distance">-- cm</span>
                        </div>
                        <div class="sensor-item">
                            <span>Battery:</span>
                            <span id="battery">-- V</span>
                        </div>
                        <div class="sensor-item">
                            <span>Line L:</span>
                            <span id="lineL">--</span>
                        </div>
                        <div class="sensor-item">
                            <span>Line M:</span>
                            <span id="lineM">--</span>
                        </div>
                        <div class="sensor-item">
                            <span>Line R:</span>
                            <span id="lineR">--</span>
                        </div>
                    </div>
                </div>

                <div class="panel" style="margin-top: 1rem;">
                    <h3>Status</h3>
                    <div id="status">Ready</div>
                </div>
            </div>
        </div>
    </div>

    <script>
        // Robot control functions
        async function sendCommand(cmd) {
            try {
                const response = await fetch(`/test1?var=${encodeURIComponent(JSON.stringify(cmd))}`);
                const result = await response.text();
                document.getElementById('status').textContent = `Command sent: ${JSON.stringify(cmd)}`;
                return result;
            } catch (error) {
                document.getElementById('status').textContent = `Error: ${error.message}`;
                console.error('Command error:', error);
            }
        }

        async function moveForward() {
            await sendCommand({N: 1, D1: 200, T1: 1000});
        }

        async function moveBackward() {
            await sendCommand({N: 2, D1: 200, T1: 1000});
        }

        async function turnLeft() {
            await sendCommand({N: 3, D1: 200, T1: 500});
        }

        async function turnRight() {
            await sendCommand({N: 4, D1: 200, T1: 500});
        }

        async function stopRobot() {
            await sendCommand({N: 100});
        }

        async function setLED(r, g, b) {
            await sendCommand({N: 5, D1: 1, D2: r, D3: g, D4: b});
        }

        async function controlServo(servo, angle) {
            await sendCommand({N: 4, D1: servo, D2: angle});
        }

        function startCamera() {
            document.getElementById('camera').src = '/stream?' + new Date().getTime();
        }

        function stopCamera() {
            document.getElementById('camera').src = '';
        }

        // Update sensor data periodically
        async function updateSensors() {
            try {
                const response = await fetch('/status');
                if (response.ok) {
                    const data = await response.json();
                    document.getElementById('distance').textContent = (data.distance || '--') + ' cm';
                    document.getElementById('battery').textContent = (data.battery || '--') + ' V';
                    document.getElementById('lineL').textContent = data.lineL || '--';
                    document.getElementById('lineM').textContent = data.lineM || '--';
                    document.getElementById('lineR').textContent = data.lineR || '--';
                }
            } catch (error) {
                console.log('Sensor update failed:', error);
            }
        }

        // Start sensor updates
        setInterval(updateSensors, 2000);

        // Initialize
        document.addEventListener('DOMContentLoaded', function() {
            startCamera();
            updateSensors();
            document.getElementById('status').textContent = 'Robot Controller Ready';
        });

        // Keyboard controls
        document.addEventListener('keydown', function(event) {
            switch(event.key) {
                case 'ArrowUp': case 'w': case 'W': moveForward(); break;
                case 'ArrowDown': case 's': case 'S': moveBackward(); break;
                case 'ArrowLeft': case 'a': case 'A': turnLeft(); break;
                case 'ArrowRight': case 'd': case 'D': turnRight(); break;
                case ' ': event.preventDefault(); stopRobot(); break;
            }
        });
    </script>
</body>
</html>
)rawliteral";

#endif