export class RobotController {
    constructor() {
        this.baseUrl = '';
        this.connected = false;
        this.sensorData = {
            ultrasonic: 0,
            battery: 0,
            lineTracking: { L: 0, M: 0, R: 0 }
        };
        this.commandQueue = [];
        this.isExecuting = false;
    }

    async connect(ip) {
        this.baseUrl = `http://${ip}`;
        
        try {
            // Test connection by getting status
            const response = await fetch(`${this.baseUrl}/status`, {
                method: 'GET'
            });
            
            if (response.ok) {
                this.connected = true;
                console.log('Connected to robot at', ip);
                return true;
            } else {
                throw new Error('Failed to connect');
            }
        } catch (error) {
            this.connected = false;
            throw new Error(`Connection failed: ${error.message}`);
        }
    }

    isConnected() {
        return this.connected;
    }

    async sendCommand(command) {
        if (!this.connected) {
            throw new Error('Not connected to robot');
        }

        try {
            const url = `${this.baseUrl}/test1?var=${encodeURIComponent(JSON.stringify(command))}`;
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
                if (!this.isExecuting) break; // Stop if requested
                
                await this.executeCommand(command);
                
                // Small delay between commands
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
            case 'moveCustom':
                await this.moveCustom(command.params[0], command.params[1]);
                break;
            case 'controlMotor':
                await this.controlMotor(command.params[0], command.params[1], command.params[2]);
                break;
            case 'controlServo':
                await this.controlServo(command.params[0], command.params[1]);
                break;
            case 'sweepServo':
                await this.sweepServo(command.params[0], command.params[1], command.params[2], command.params[3]);
                break;
            case 'setLEDColor':
                await this.setLEDColor(command.params[0], command.params[1], command.params[2]);
                break;
            case 'blinkLED':
                await this.blinkLED(command.params[0], command.params[1], command.params[2], command.params[3], command.params[4]);
                break;
            case 'wait':
                await this.delay(command.params[0] * 1000);
                break;
            default:
                console.warn('Unknown command:', command.method);
        }
    }

    // Movement commands
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

    async moveCustom(direction, speed) {
        const cmd = { N: parseInt(direction), D1: speed };
        await this.sendCommand(cmd);
    }

    // Motor control
    async controlMotor(motor, direction, speed) {
        const cmd = { N: 1, D1: motor, D2: direction, D3: speed };
        await this.sendCommand(cmd);
    }

    // Servo control
    async controlServo(servo, angle) {
        const cmd = { N: 4, D1: servo, D2: angle };
        await this.sendCommand(cmd);
    }

    async sweepServo(servo, startAngle, endAngle, delayMs) {
        const step = startAngle < endAngle ? 10 : -10;
        const steps = Math.abs(endAngle - startAngle) / Math.abs(step);
        
        for (let i = 0; i <= steps; i++) {
            const angle = startAngle + (step * i);
            await this.controlServo(servo, angle);
            await this.delay(delayMs);
        }
    }

    // LED control
    async setLEDColor(r, g, b) {
        const cmd = { N: 5, D1: 1, D2: r, D3: g, D4: b };
        await this.sendCommand(cmd);
    }

    async blinkLED(r, g, b, count, delayMs) {
        for (let i = 0; i < count; i++) {
            await this.setLEDColor(r, g, b);
            await this.delay(delayMs);
            await this.setLEDColor(0, 0, 0);
            await this.delay(delayMs);
        }
    }

    // Sensor reading
    async readSensors() {
        try {
            const response = await fetch(`${this.baseUrl}/status`);
            if (response.ok) {
                const data = await response.json();
                // Update sensor data based on response
                // Note: The actual sensor data format may need adjustment based on robot response
                return data;
            }
        } catch (error) {
            console.error('Failed to read sensors:', error);
        }
        return null;
    }

    async readUltrasonic() {
        const data = await this.readSensors();
        return data ? (data.ultrasonic || 0) : 0;
    }

    async readLineTracking(sensor) {
        const data = await this.readSensors();
        if (data && data.lineTracking) {
            return data.lineTracking[sensor] || 0;
        }
        return 0;
    }

    async readBattery() {
        const data = await this.readSensors();
        return data ? (data.battery || 0) : 0;
    }

    stopAllCommands() {
        this.isExecuting = false;
        this.commandQueue = [];
        this.stop(); // Send stop command to robot
    }

    delay(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }

    getSensorData() {
        return this.sensorData;
    }

    async updateSensorData() {
        try {
            const data = await this.readSensors();
            if (data) {
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
}