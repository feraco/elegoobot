// Robot Controller Class
class RobotController {
    constructor() {
        this.baseUrl = '';
        this.connected = true; // Always connected when hosted on ESP32
        this.sensorData = {
            ultrasonic: 0,
            battery: 0,
            lineTracking: { L: 0, M: 0, R: 0 }
        };
        this.commandQueue = [];
        this.isExecuting = false;
    }

    isConnected() {
        return this.connected;
    }

    async sendCommand(command) {
        try {
            const url = `/test1?var=${encodeURIComponent(JSON.stringify(command))}`;
            const response = await fetch(url, { method: 'GET' });
            
            if (!response.ok) {
                throw new Error(`Command failed: ${response.status}`);
            }
            
            return await response.text();
        } catch (error) {
            console.error('Command error:', error);
            throw error;
        }
    }

    async executeCommands(commands) {
        if (this.isExecuting) return;
        
        this.isExecuting = true;
        this.commandQueue = [...commands];

        try {
            for (const command of this.commandQueue) {
                if (!this.isExecuting) break;
                
                await this.executeCommand(command);
                await this.delay(50);
            }
        } finally {
            this.isExecuting = false;
            this.commandQueue = [];
        }
    }

    async executeCommand(command) {
        if (!command || !command.method) return;

        switch (command.method) {
            case 'moveForward':
                await this.moveForward(command.params[0], command.params[1]);
                break;
            case 'moveBackward':
                await this.moveBackward(command.params[0], command.params[1]);
                break;
            case 'turnLeft':
                await this.turnLeft(command.params[0]);
                break;
            case 'turnRight':
                await this.turnRight(command.params[0]);
                break;
            case 'stop':
                await this.stop();
                break;
            case 'controlServo':
                await this.controlServo(command.params[0], command.params[1]);
                break;
            case 'setLEDColor':
                await this.setLEDColor(command.params[0], command.params[1], command.params[2]);
                break;
            case 'wait':
                await this.delay(command.params[0] * 1000);
                break;
        }
    }

    async moveForward(speed = 200, time = 1) {
        const cmd = { N: 1, D1: speed, T1: time * 1000 };
        await this.sendCommand(cmd);
        await this.delay(time * 1000);
    }

    async moveBackward(speed = 200, time = 1) {
        const cmd = { N: 2, D1: speed, T1: time * 1000 };
        await this.sendCommand(cmd);
        await this.delay(time * 1000);
    }

    async turnLeft(time = 1) {
        const cmd = { N: 3, D1: 200, T1: time * 1000 };
        await this.sendCommand(cmd);
        await this.delay(time * 1000);
    }

    async turnRight(time = 1) {
        const cmd = { N: 4, D1: 200, T1: time * 1000 };
        await this.sendCommand(cmd);
        await this.delay(time * 1000);
    }

    async stop() {
        const cmd = { N: 100 };
        await this.sendCommand(cmd);
    }

    async controlServo(servo, angle) {
        const cmd = { N: 4, D1: servo, D2: angle };
        await this.sendCommand(cmd);
    }

    async setLEDColor(r, g, b) {
        const cmd = { N: 5, D1: 1, D2: r, D3: g, D4: b };
        await this.sendCommand(cmd);
    }

    stopAllCommands() {
        this.isExecuting = false;
        this.commandQueue = [];
        this.stop();
    }

    delay(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }

    async updateSensorData() {
        try {
            const response = await fetch('/status');
            if (response.ok) {
                const data = await response.json();
                // Update sensor data - you may need to adjust based on actual response format
                this.sensorData = {
                    ultrasonic: data.ultrasonic || 0,
                    battery: data.battery || 0,
                    lineTracking: data.lineTracking || { L: 0, M: 0, R: 0 }
                };
            }
        } catch (error) {
            console.error('Failed to update sensor data:', error);
        }
    }

    getSensorData() {
        return this.sensorData;
    }
}

// Blockly Workspace Class
class BlocklyWorkspace {
    constructor() {
        this.workspace = null;
        this.init();
    }

    init() {
        // Define custom blocks
        this.defineBlocks();

        const toolbox = {
            kind: 'categoryToolbox',
            contents: [
                {
                    kind: 'category',
                    name: 'Movement',
                    colour: '#4CAF50',
                    contents: [
                        { kind: 'block', type: 'robot_move_forward' },
                        { kind: 'block', type: 'robot_move_backward' },
                        { kind: 'block', type: 'robot_turn_left' },
                        { kind: 'block', type: 'robot_turn_right' },
                        { kind: 'block', type: 'robot_stop' }
                    ]
                },
                {
                    kind: 'category',
                    name: 'Servo',
                    colour: '#FF9800',
                    contents: [
                        { kind: 'block', type: 'robot_servo_control' }
                    ]
                },
                {
                    kind: 'category',
                    name: 'Lighting',
                    colour: '#9C27B0',
                    contents: [
                        { kind: 'block', type: 'robot_led_color' },
                        { kind: 'block', type: 'robot_led_off' }
                    ]
                },
                {
                    kind: 'category',
                    name: 'Control',
                    colour: '#607D8B',
                    contents: [
                        { kind: 'block', type: 'robot_wait' },
                        { kind: 'block', type: 'robot_repeat' }
                    ]
                }
            ]
        };

        this.workspace = Blockly.inject('blocklyDiv', {
            toolbox: toolbox,
            grid: {
                spacing: 20,
                length: 3,
                colour: '#ccc',
                snap: true
            },
            zoom: {
                controls: true,
                wheel: true,
                startScale: 1.0,
                maxScale: 3,
                minScale: 0.3,
                scaleSpeed: 1.2
            },
            trashcan: true,
            sounds: false
        });
    }

    defineBlocks() {
        // Movement blocks
        Blockly.Blocks['robot_move_forward'] = {
            init: function() {
                this.appendDummyInput()
                    .appendField("move forward speed")
                    .appendField(new Blockly.FieldNumber(200, 0, 255), "SPEED")
                    .appendField("for")
                    .appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME")
                    .appendField("seconds");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#4CAF50');
            }
        };

        Blockly.Blocks['robot_move_backward'] = {
            init: function() {
                this.appendDummyInput()
                    .appendField("move backward speed")
                    .appendField(new Blockly.FieldNumber(200, 0, 255), "SPEED")
                    .appendField("for")
                    .appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME")
                    .appendField("seconds");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#4CAF50');
            }
        };

        Blockly.Blocks['robot_turn_left'] = {
            init: function() {
                this.appendDummyInput()
                    .appendField("turn left for")
                    .appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME")
                    .appendField("seconds");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#4CAF50');
            }
        };

        Blockly.Blocks['robot_turn_right'] = {
            init: function() {
                this.appendDummyInput()
                    .appendField("turn right for")
                    .appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME")
                    .appendField("seconds");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#4CAF50');
            }
        };

        Blockly.Blocks['robot_stop'] = {
            init: function() {
                this.appendDummyInput()
                    .appendField("stop robot");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#4CAF50');
            }
        };

        // Servo blocks
        Blockly.Blocks['robot_servo_control'] = {
            init: function() {
                this.appendDummyInput()
                    .appendField("move servo")
                    .appendField(new Blockly.FieldDropdown([
                        ["horizontal", "1"],
                        ["vertical", "2"]
                    ]), "SERVO")
                    .appendField("to angle")
                    .appendField(new Blockly.FieldNumber(90, 0, 180), "ANGLE");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#FF9800');
            }
        };

        // LED blocks
        Blockly.Blocks['robot_led_color'] = {
            init: function() {
                this.appendDummyInput()
                    .appendField("set LED color to")
                    .appendField(new Blockly.FieldColour('#ff0000'), "COLOR");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#9C27B0');
            }
        };

        Blockly.Blocks['robot_led_off'] = {
            init: function() {
                this.appendDummyInput()
                    .appendField("turn LED off");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#9C27B0');
            }
        };

        // Control blocks
        Blockly.Blocks['robot_wait'] = {
            init: function() {
                this.appendDummyInput()
                    .appendField("wait")
                    .appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME")
                    .appendField("seconds");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#607D8B');
            }
        };

        Blockly.Blocks['robot_repeat'] = {
            init: function() {
                this.appendDummyInput()
                    .appendField("repeat")
                    .appendField(new Blockly.FieldNumber(3, 1, 100), "TIMES")
                    .appendField("times");
                this.appendStatementInput("DO")
                    .setCheck(null)
                    .appendField("do");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#607D8B');
            }
        };

        // Define generators
        Blockly.JavaScript['robot_move_forward'] = function(block) {
            const speed = block.getFieldValue('SPEED');
            const time = block.getFieldValue('TIME');
            return `robot.moveForward(${speed}, ${time});\n`;
        };

        Blockly.JavaScript['robot_move_backward'] = function(block) {
            const speed = block.getFieldValue('SPEED');
            const time = block.getFieldValue('TIME');
            return `robot.moveBackward(${speed}, ${time});\n`;
        };

        Blockly.JavaScript['robot_turn_left'] = function(block) {
            const time = block.getFieldValue('TIME');
            return `robot.turnLeft(${time});\n`;
        };

        Blockly.JavaScript['robot_turn_right'] = function(block) {
            const time = block.getFieldValue('TIME');
            return `robot.turnRight(${time});\n`;
        };

        Blockly.JavaScript['robot_stop'] = function(block) {
            return `robot.stop();\n`;
        };

        Blockly.JavaScript['robot_servo_control'] = function(block) {
            const servo = block.getFieldValue('SERVO');
            const angle = block.getFieldValue('ANGLE');
            return `robot.controlServo(${servo}, ${angle});\n`;
        };

        Blockly.JavaScript['robot_led_color'] = function(block) {
            const color = block.getFieldValue('COLOR');
            const r = parseInt(color.substr(1, 2), 16);
            const g = parseInt(color.substr(3, 2), 16);
            const b = parseInt(color.substr(5, 2), 16);
            return `robot.setLEDColor(${r}, ${g}, ${b});\n`;
        };

        Blockly.JavaScript['robot_led_off'] = function(block) {
            return `robot.setLEDColor(0, 0, 0);\n`;
        };

        Blockly.JavaScript['robot_wait'] = function(block) {
            const time = block.getFieldValue('TIME');
            return `robot.wait(${time});\n`;
        };

        Blockly.JavaScript['robot_repeat'] = function(block) {
            const times = block.getFieldValue('TIMES');
            const statements = Blockly.JavaScript.statementToCode(block, 'DO');
            return `for (let i = 0; i < ${times}; i++) {\n${statements}}\n`;
        };
    }

    generateCode() {
        return Blockly.JavaScript.workspaceToCode(this.workspace);
    }

    generateCommands() {
        const code = this.generateCode();
        return this.parseCodeToCommands(code);
    }

    parseCodeToCommands(code) {
        const commands = [];
        const lines = code.split('\n').filter(line => line.trim());
        
        for (const line of lines) {
            const trimmed = line.trim();
            if (trimmed.startsWith('robot.')) {
                commands.push(this.parseRobotCommand(trimmed));
            }
        }
        
        return commands;
    }

    parseRobotCommand(line) {
        const match = line.match(/robot\.(\w+)\((.*)\)/);
        if (!match) return null;

        const [, method, params] = match;
        const parsedParams = this.parseParameters(params);

        return {
            method,
            params: parsedParams
        };
    }

    parseParameters(paramString) {
        if (!paramString.trim()) return [];
        
        return paramString.split(',').map(param => {
            const trimmed = param.trim();
            if (trimmed.startsWith('"') && trimmed.endsWith('"')) {
                return trimmed.slice(1, -1);
            }
            const num = parseFloat(trimmed);
            return isNaN(num) ? trimmed : num;
        });
    }

    addChangeListener(callback) {
        this.workspace.addChangeListener(callback);
    }
}

// UI Manager Class
class UIManager {
    constructor(robotController) {
        this.robotController = robotController;
    }

    init() {
        this.updateConnectionStatus(true);
        this.setupCameraFeed();
    }

    updateConnectionStatus(connected) {
        const statusIndicator = document.getElementById('statusIndicator');
        const statusText = document.getElementById('statusText');

        if (connected) {
            statusIndicator.style.background = '#10b981';
            statusText.textContent = 'Connected to Robot';
        } else {
            statusIndicator.style.background = '#ef4444';
            statusText.textContent = 'Disconnected';
        }
    }

    setupCameraFeed() {
        const cameraFeed = document.getElementById('cameraFeed');
        cameraFeed.style.display = 'block';
    }

    startCamera() {
        const cameraFeed = document.getElementById('cameraFeed');
        cameraFeed.src = '/stream';
        cameraFeed.style.display = 'block';
    }

    stopCamera() {
        const cameraFeed = document.getElementById('cameraFeed');
        cameraFeed.src = '';
    }

    async updateSensorData() {
        try {
            await this.robotController.updateSensorData();
            const data = this.robotController.getSensorData();

            document.getElementById('ultrasonicValue').textContent = data.ultrasonic.toFixed(1);
            document.getElementById('batteryValue').textContent = data.battery.toFixed(2);
            document.getElementById('trackingL').textContent = data.lineTracking.L;
            document.getElementById('trackingM').textContent = data.lineTracking.M;
            document.getElementById('trackingR').textContent = data.lineTracking.R;

            this.updateTrackingSensorColors(data.lineTracking);
        } catch (error) {
            console.error('Failed to update sensor display:', error);
        }
    }

    updateTrackingSensorColors(tracking) {
        const threshold = 500;
        
        ['L', 'M', 'R'].forEach(sensor => {
            const element = document.getElementById(`tracking${sensor}`);
            const value = tracking[sensor];
            
            if (value > threshold) {
                element.style.backgroundColor = '#ef4444';
                element.style.color = 'white';
            } else {
                element.style.backgroundColor = '#10b981';
                element.style.color = 'white';
            }
        });
    }
}

// Main App Class
class App {
    constructor() {
        this.workspace = null;
        this.robotController = null;
        this.uiManager = null;
        this.isRunning = false;
    }

    async init() {
        try {
            this.robotController = new RobotController();
            this.uiManager = new UIManager(this.robotController);
            this.workspace = new BlocklyWorkspace();

            this.setupEventListeners();
            this.uiManager.init();

            console.log('Robot Car Controller initialized successfully');
        } catch (error) {
            console.error('Failed to initialize app:', error);
        }
    }

    setupEventListeners() {
        // Camera controls
        document.getElementById('startCamera').addEventListener('click', () => {
            this.uiManager.startCamera();
        });

        document.getElementById('stopCamera').addEventListener('click', () => {
            this.uiManager.stopCamera();
        });

        // Code execution
        document.getElementById('runCode').addEventListener('click', () => {
            this.runCode();
        });

        document.getElementById('stopCode').addEventListener('click', () => {
            this.stopCode();
        });

        // Workspace changes
        this.workspace.addChangeListener(() => {
            this.updateCodeOutput();
        });
    }

    updateCodeOutput() {
        const code = this.workspace.generateCode();
        document.getElementById('codeOutput').textContent = code;
    }

    async runCode() {
        if (this.isRunning) return;
        
        this.isRunning = true;
        document.getElementById('runCode').disabled = true;
        document.getElementById('stopCode').disabled = false;

        try {
            const commands = this.workspace.generateCommands();
            await this.robotController.executeCommands(commands);
        } catch (error) {
            console.error('Error running code:', error);
            alert('Error executing commands: ' + error.message);
        } finally {
            this.isRunning = false;
            document.getElementById('runCode').disabled = false;
            document.getElementById('stopCode').disabled = true;
        }
    }

    stopCode() {
        this.robotController.stopAllCommands();
        this.isRunning = false;
        document.getElementById('runCode').disabled = false;
        document.getElementById('stopCode').disabled = true;
    }

    startSensorUpdates() {
        setInterval(() => {
            this.uiManager.updateSensorData();
        }, 1000);
    }
}

// Initialize app when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    const app = new App();
    app.init();
    app.startSensorUpdates();
});