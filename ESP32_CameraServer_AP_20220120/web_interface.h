/*
 * Web Interface HTML Content
 * This file contains the robot control interface
 */

#ifndef _WEB_INTERFACE_H_
#define _WEB_INTERFACE_H_

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Robot Car Controller</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            color: #333;
        }
        .header { 
            background: rgba(255, 255, 255, 0.95); 
            backdrop-filter: blur(10px);
            padding: 1rem; 
            text-align: center;
            box-shadow: 0 2px 8px rgba(0,0,0,0.1);
        }
        .title { 
            font-size: 1.5rem; 
            font-weight: bold; 
            color: #2d3748; 
        }
        .main { 
            max-width: 1200px; 
            margin: 2rem auto; 
            padding: 0 1rem; 
        }
        .workspace { 
            display: grid; 
            grid-template-columns: 1fr 350px; 
            gap: 1.5rem; 
        }
        .panel { 
            background: rgba(255, 255, 255, 0.95); 
            backdrop-filter: blur(10px);
            border-radius: 16px; 
            padding: 1.5rem; 
            box-shadow: 0 8px 32px rgba(0,0,0,0.1);
        }
        .panel h3 {
            margin-bottom: 1rem;
            color: #2d3748;
            font-weight: 600;
        }
        .control-grid { 
            display: grid; 
            grid-template-columns: 1fr 1fr; 
            gap: 0.75rem; 
            margin-bottom: 1.5rem;
        }
        .control-btn { 
            padding: 1.2rem; 
            border: none; 
            border-radius: 12px; 
            font-size: 1.1rem; 
            font-weight: bold; 
            cursor: pointer; 
            transition: all 0.2s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 0.5rem;
        }
        .control-btn:hover { transform: translateY(-2px); box-shadow: 0 4px 12px rgba(0,0,0,0.2); }
        .move-forward { background: #10b981; color: white; }
        .move-backward { background: #6b7280; color: white; }
        .turn-left { background: #f59e0b; color: white; }
        .turn-right { background: #f97316; color: white; }
        .stop-btn { background: #ef4444; color: white; grid-column: span 2; font-size: 1.3rem; }
        .camera { 
            width: 100%; 
            max-width: 320px; 
            height: 240px; 
            border-radius: 12px; 
            border: 2px solid #e5e7eb;
            background: #f3f4f6;
            object-fit: cover;
        }
        .camera-controls { 
            display: flex; 
            gap: 0.5rem; 
            justify-content: center; 
            margin-top: 1rem;
        }
        .btn { 
            padding: 0.5rem 1rem; 
            border: none; 
            border-radius: 8px; 
            cursor: pointer; 
            font-weight: 500; 
            transition: all 0.2s ease;
        }
        .btn-primary { background: #3b82f6; color: white; }
        .btn-danger { background: #ef4444; color: white; }
        .btn:hover { opacity: 0.9; transform: translateY(-1px); }
        .led-controls { margin-top: 1.5rem; }
        .color-grid { 
            display: grid; 
            grid-template-columns: repeat(4, 1fr); 
            gap: 0.5rem; 
            margin-top: 0.5rem;
        }
        .color-btn { 
            width: 50px; 
            height: 50px; 
            border: 3px solid #fff; 
            border-radius: 12px; 
            cursor: pointer; 
            transition: all 0.2s ease;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .color-btn:hover { transform: scale(1.1); box-shadow: 0 4px 8px rgba(0,0,0,0.2); }
        .servo-controls { margin-top: 1.5rem; }
        .servo-item { 
            display: flex; 
            align-items: center; 
            gap: 1rem; 
            margin-bottom: 1rem;
        }
        .servo-item label { font-weight: 500; min-width: 80px; }
        .servo-slider { 
            flex: 1; 
            height: 8px; 
            border-radius: 4px; 
            background: #e5e7eb;
            outline: none;
            cursor: pointer;
        }
        .sensor-grid { 
            display: grid; 
            gap: 0.75rem; 
            margin-top: 1rem; 
        }
        .sensor-item { 
            display: flex; 
            justify-content: space-between; 
            align-items: center;
            padding: 0.75rem; 
            background: #f8fafc; 
            border-radius: 8px; 
            border: 1px solid #e5e7eb;
        }
        .sensor-item label { font-weight: 500; color: #4b5563; }
        .sensor-value { 
            font-weight: 600; 
            color: #1f2937; 
            font-family: 'Monaco', monospace;
            background: #fff;
            padding: 0.25rem 0.5rem;
            border-radius: 4px;
        }
        .status { 
            margin-top: 1rem; 
            padding: 0.75rem; 
            background: #f0f9ff; 
            border-radius: 8px; 
            border-left: 4px solid #3b82f6;
            font-weight: 500;
        }
        @media (max-width: 768px) { 
            .workspace { grid-template-columns: 1fr; }
            .main { padding: 1rem; }
        }
    </style>
</head>
<body>
    <div class="header">
        <h1 class="title">🤖 Smart Robot Car Controller</h1>
    </div>
    <div class="main">
        <div class="workspace">
            <div class="panel">
                <h3>🎮 Manual Controls</h3>
                <div class="control-grid">
                    <button class="control-btn move-forward" onclick="moveForward()">
                        ⬆️ Forward
                    </button>
                    <button class="control-btn move-backward" onclick="moveBackward()">
                        ⬇️ Backward
                    </button>
                    <button class="control-btn turn-left" onclick="turnLeft()">
                        ⬅️ Left
                    </button>
                    <button class="control-btn turn-right" onclick="turnRight()">
                        ➡️ Right
                    </button>
                    <button class="control-btn stop-btn" onclick="stopRobot()">
                        ⏹️ STOP
                    </button>
                </div>
                
                <div class="led-controls">
                    <h4>💡 LED Colors</h4>
                    <div class="color-grid">
                        <div class="color-btn" style="background: #ef4444;" onclick="setLED(255,0,0)" title="Red"></div>
                        <div class="color-btn" style="background: #10b981;" onclick="setLED(0,255,0)" title="Green"></div>
                        <div class="color-btn" style="background: #3b82f6;" onclick="setLED(0,0,255)" title="Blue"></div>
                        <div class="color-btn" style="background: #f59e0b;" onclick="setLED(255,255,0)" title="Yellow"></div>
                        <div class="color-btn" style="background: #8b5cf6;" onclick="setLED(255,0,255)" title="Purple"></div>
                        <div class="color-btn" style="background: #06b6d4;" onclick="setLED(0,255,255)" title="Cyan"></div>
                        <div class="color-btn" style="background: #ffffff; border-color: #d1d5db;" onclick="setLED(255,255,255)" title="White"></div>
                        <div class="color-btn" style="background: #000000;" onclick="setLED(0,0,0)" title="Off"></div>
                    </div>
                </div>

                <div class="servo-controls">
                    <h4>🔄 Camera Control</h4>
                    <div class="servo-item">
                        <label>Horizontal:</label>
                        <input type="range" class="servo-slider" id="servoH" min="10" max="170" value="90" onchange="controlServo(1, this.value)">
                        <span id="servoHValue">90°</span>
                    </div>
                    <div class="servo-item">
                        <label>Vertical:</label>
                        <input type="range" class="servo-slider" id="servoV" min="30" max="110" value="70" onchange="controlServo(2, this.value)">
                        <span id="servoVValue">70°</span>
                    </div>
                </div>
            </div>
            
            <div>
                <div class="panel">
                    <h3>📹 Camera Feed</h3>
                    <div style="text-align: center;">
                        <img id="camera" class="camera" alt="Camera Loading..." onerror="handleCameraError()">
                        <div class="camera-controls">
                            <button class="btn btn-primary" onclick="startCamera()">Start Camera</button>
                            <button class="btn btn-danger" onclick="stopCamera()">Stop Camera</button>
                        </div>
                    </div>
                </div>
                
                <div class="panel" style="margin-top: 1rem;">
                    <h3>📊 Sensor Data</h3>
                    <div class="sensor-grid">
                        <div class="sensor-item">
                            <label>Distance:</label>
                            <span class="sensor-value" id="distance">-- cm</span>
                        </div>
                        <div class="sensor-item">
                            <label>Battery:</label>
                            <span class="sensor-value" id="battery">-- V</span>
                        </div>
                        <div class="sensor-item">
                            <label>Line Left:</label>
                            <span class="sensor-value" id="lineL">--</span>
                        </div>
                        <div class="sensor-item">
                            <label>Line Center:</label>
                            <span class="sensor-value" id="lineM">--</span>
                        </div>
                        <div class="sensor-item">
                            <label>Line Right:</label>
                            <span class="sensor-value" id="lineR">--</span>
                        </div>
                    </div>
                </div>

                <div class="panel" style="margin-top: 1rem;">
                    <h3>ℹ️ Status</h3>
                    <div class="status" id="status">Robot Controller Ready</div>
                </div>
            </div>
        </div>
    </div>

    <script>
        // Robot control functions
        async function sendCommand(cmd) {
            try {
                const response = await fetch(`/test1?var=${encodeURIComponent(JSON.stringify(cmd))}`);
                if (response.ok) {
                    const result = await response.text();
                    document.getElementById('status').textContent = `✅ Command sent: ${JSON.stringify(cmd)}`;
                    return result;
                } else {
                    throw new Error(`HTTP ${response.status}`);
                }
            } catch (error) {
                document.getElementById('status').textContent = `❌ Error: ${error.message}`;
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
            await sendCommand({N: 3, D1: 200, T1: 800});
        }

        async function turnRight() {
            await sendCommand({N: 4, D1: 200, T1: 800});
        }

        async function stopRobot() {
            await sendCommand({N: 100});
        }

        async function setLED(r, g, b) {
            await sendCommand({N: 5, D1: 1, D2: r, D3: g, D4: b});
        }

        async function controlServo(servo, angle) {
            await sendCommand({N: 4, D1: servo, D2: angle});
            document.getElementById(`servo${servo === 1 ? 'H' : 'V'}Value`).textContent = angle + '°';
        }

        function startCamera() {
            const camera = document.getElementById('camera');
            camera.src = '/stream?' + new Date().getTime();
            document.getElementById('status').textContent = '📹 Camera started';
        }

        function stopCamera() {
            const camera = document.getElementById('camera');
            camera.src = '';
            document.getElementById('status').textContent = '📹 Camera stopped';
        }

        function handleCameraError() {
            console.log('Camera stream failed, trying capture endpoint');
            const camera = document.getElementById('camera');
            if (camera.src.includes('/stream')) {
                camera.src = '/capture?' + new Date().getTime();
            }
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
                } else {
                    // If status endpoint doesn't exist, show placeholder data
                    document.getElementById('distance').textContent = '-- cm';
                    document.getElementById('battery').textContent = '-- V';
                    document.getElementById('lineL').textContent = '--';
                    document.getElementById('lineM').textContent = '--';
                    document.getElementById('lineR').textContent = '--';
                }
            } catch (error) {
                console.log('Sensor update failed:', error);
            }
        }

        // Keyboard controls
        document.addEventListener('keydown', function(event) {
            switch(event.key.toLowerCase()) {
                case 'arrowup': case 'w': moveForward(); break;
                case 'arrowdown': case 's': moveBackward(); break;
                case 'arrowleft': case 'a': turnLeft(); break;
                case 'arrowright': case 'd': turnRight(); break;
                case ' ': event.preventDefault(); stopRobot(); break;
            }
        });

        // Initialize when page loads
        document.addEventListener('DOMContentLoaded', function() {
            document.getElementById('status').textContent = '🤖 Robot Controller Ready - Use buttons or WASD keys';
            startCamera();
            updateSensors();
            
            // Update sensor data every 3 seconds
            setInterval(updateSensors, 3000);
        });
    </script>
</body>
</html>
)rawliteral";

#endif