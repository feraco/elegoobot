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
        .blockly-area { height: 500px; }
        .controls { display: flex; gap: 0.5rem; margin-top: 1rem; }
        .btn { padding: 0.5rem 1rem; border: none; border-radius: 4px; cursor: pointer; font-weight: 500; }
        .btn-primary { background: #007bff; color: white; }
        .btn-danger { background: #dc3545; color: white; }
        .btn:hover { opacity: 0.9; }
        .camera { width: 100%; max-width: 280px; height: 210px; border-radius: 4px; }
        .sensor-grid { display: grid; gap: 0.5rem; margin-top: 1rem; }
        .sensor-item { display: flex; justify-content: space-between; padding: 0.5rem; background: #f8f9fa; border-radius: 4px; }
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
                <h3>Programming Blocks</h3>
                <div id="blocklyDiv" class="blockly-area"></div>
                <div class="controls">
                    <button id="runCode" class="btn btn-primary">Run Code</button>
                    <button id="stopCode" class="btn btn-danger">Stop</button>
                </div>
            </div>
            <div>
                <div class="panel">
                    <h3>Camera</h3>
                    <img id="camera" src="/stream" class="camera" alt="Camera">
                </div>
                <div class="panel" style="margin-top: 1rem;">
                    <h3>Sensors</h3>
                    <div class="sensor-grid">
                        <div class="sensor-item">
                            <span>Distance:</span>
                            <span id="distance">--</span>
                        </div>
                        <div class="sensor-item">
                            <span>Battery:</span>
                            <span id="battery">--</span>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
    <script src="https://unpkg.com/blockly@10.4.3/blockly.min.js"></script>
    <script>
        class RobotController {
            constructor() { this.isExecuting = false; }
            async sendCommand(cmd) {
                const url = `/test1?var=${encodeURIComponent(JSON.stringify(cmd))}`;
                const response = await fetch(url);
                return response.text();
            }
            async moveForward(speed, time) {
                await this.sendCommand({N: 1, D1: speed, T1: time * 1000});
                await this.delay(time * 1000);
            }
            async moveBackward(speed, time) {
                await this.sendCommand({N: 2, D1: speed, T1: time * 1000});
                await this.delay(time * 1000);
            }
            async turnLeft(time) {
                await this.sendCommand({N: 3, D1: 200, T1: time * 1000});
                await this.delay(time * 1000);
            }
            async turnRight(time) {
                await this.sendCommand({N: 4, D1: 200, T1: time * 1000});
                await this.delay(time * 1000);
            }
            async stop() { await this.sendCommand({N: 100}); }
            async setLED(r, g, b) { await this.sendCommand({N: 5, D1: 1, D2: r, D3: g, D4: b}); }
            async controlServo(servo, angle) { await this.sendCommand({N: 4, D1: servo, D2: angle}); }
            delay(ms) { return new Promise(resolve => setTimeout(resolve, ms)); }
            stopAll() { this.isExecuting = false; this.stop(); }
        }

        const robot = new RobotController();

        // Define blocks
        Blockly.Blocks['move_forward'] = {
            init: function() {
                this.appendDummyInput().appendField("move forward speed").appendField(new Blockly.FieldNumber(200, 0, 255), "SPEED").appendField("for").appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME").appendField("sec");
                this.setPreviousStatement(true); this.setNextStatement(true); this.setColour(120);
            }
        };
        Blockly.Blocks['move_backward'] = {
            init: function() {
                this.appendDummyInput().appendField("move backward speed").appendField(new Blockly.FieldNumber(200, 0, 255), "SPEED").appendField("for").appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME").appendField("sec");
                this.setPreviousStatement(true); this.setNextStatement(true); this.setColour(120);
            }
        };
        Blockly.Blocks['turn_left'] = {
            init: function() {
                this.appendDummyInput().appendField("turn left for").appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME").appendField("sec");
                this.setPreviousStatement(true); this.setNextStatement(true); this.setColour(120);
            }
        };
        Blockly.Blocks['turn_right'] = {
            init: function() {
                this.appendDummyInput().appendField("turn right for").appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME").appendField("sec");
                this.setPreviousStatement(true); this.setNextStatement(true); this.setColour(120);
            }
        };
        Blockly.Blocks['stop_robot'] = {
            init: function() {
                this.appendDummyInput().appendField("stop robot");
                this.setPreviousStatement(true); this.setNextStatement(true); this.setColour(120);
            }
        };
        Blockly.Blocks['set_led'] = {
            init: function() {
                this.appendDummyInput().appendField("set LED").appendField(new Blockly.FieldColour("#ff0000"), "COLOR");
                this.setPreviousStatement(true); this.setNextStatement(true); this.setColour(290);
            }
        };
        Blockly.Blocks['wait_time'] = {
            init: function() {
                this.appendDummyInput().appendField("wait").appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME").appendField("sec");
                this.setPreviousStatement(true); this.setNextStatement(true); this.setColour(210);
            }
        };

        // Generators
        Blockly.JavaScript['move_forward'] = function(block) {
            const speed = block.getFieldValue('SPEED');
            const time = block.getFieldValue('TIME');
            return `await robot.moveForward(${speed}, ${time});\n`;
        };
        Blockly.JavaScript['move_backward'] = function(block) {
            const speed = block.getFieldValue('SPEED');
            const time = block.getFieldValue('TIME');
            return `await robot.moveBackward(${speed}, ${time});\n`;
        };
        Blockly.JavaScript['turn_left'] = function(block) {
            const time = block.getFieldValue('TIME');
            return `await robot.turnLeft(${time});\n`;
        };
        Blockly.JavaScript['turn_right'] = function(block) {
            const time = block.getFieldValue('TIME');
            return `await robot.turnRight(${time});\n`;
        };
        Blockly.JavaScript['stop_robot'] = function(block) {
            return `await robot.stop();\n`;
        };
        Blockly.JavaScript['set_led'] = function(block) {
            const color = block.getFieldValue('COLOR');
            const r = parseInt(color.substr(1, 2), 16);
            const g = parseInt(color.substr(3, 2), 16);
            const b = parseInt(color.substr(5, 2), 16);
            return `await robot.setLED(${r}, ${g}, ${b});\n`;
        };
        Blockly.JavaScript['wait_time'] = function(block) {
            const time = block.getFieldValue('TIME');
            return `await robot.delay(${time * 1000});\n`;
        };

        // Initialize workspace
        const workspace = Blockly.inject('blocklyDiv', {
            toolbox: {
                kind: 'categoryToolbox',
                contents: [
                    { kind: 'category', name: 'Movement', colour: 120, contents: [
                        { kind: 'block', type: 'move_forward' },
                        { kind: 'block', type: 'move_backward' },
                        { kind: 'block', type: 'turn_left' },
                        { kind: 'block', type: 'turn_right' },
                        { kind: 'block', type: 'stop_robot' }
                    ]},
                    { kind: 'category', name: 'LED', colour: 290, contents: [
                        { kind: 'block', type: 'set_led' }
                    ]},
                    { kind: 'category', name: 'Control', colour: 210, contents: [
                        { kind: 'block', type: 'wait_time' }
                    ]}
                ]
            },
            grid: { spacing: 20, length: 3, colour: '#ccc', snap: true },
            zoom: { controls: true, wheel: true, startScale: 1.0 },
            trashcan: true
        });

        // Event handlers
        document.getElementById('runCode').addEventListener('click', async () => {
            if (robot.isExecuting) return;
            robot.isExecuting = true;
            document.getElementById('runCode').disabled = true;
            try {
                const code = Blockly.JavaScript.workspaceToCode(workspace);
                await eval(`(async () => { ${code} })()`);
            } catch (error) {
                console.error('Error:', error);
                alert('Error: ' + error.message);
            } finally {
                robot.isExecuting = false;
                document.getElementById('runCode').disabled = false;
            }
        });

        document.getElementById('stopCode').addEventListener('click', () => {
            robot.stopAll();
            document.getElementById('runCode').disabled = false;
        });
    </script>
</body>
</html>
)rawliteral";

#endif