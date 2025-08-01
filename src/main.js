import { BlocklyWorkspace } from './blockly/workspace.js';
import { RobotController } from './robot/controller.js';
import { UIManager } from './ui/manager.js';

class App {
    constructor() {
        this.workspace = null;
        this.robotController = null;
        this.uiManager = null;
        this.isRunning = false;
    }

    async init() {
        try {
            // Initialize components
            this.robotController = new RobotController();
            this.uiManager = new UIManager(this.robotController);
            this.workspace = new BlocklyWorkspace();

            // Setup event listeners
            this.setupEventListeners();

            // Initialize UI
            this.uiManager.init();

            console.log('Robot Car Controller initialized successfully');
        } catch (error) {
            console.error('Failed to initialize app:', error);
        }
    }

    setupEventListeners() {
        // Connect button
        document.getElementById('connectBtn').addEventListener('click', () => {
            this.handleConnect();
        });

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

    async handleConnect() {
        const ipInput = document.getElementById('robotIP');
        const ip = ipInput.value.trim();

        if (!ip) {
            alert('Please enter a valid IP address');
            return;
        }

        try {
            await this.robotController.connect(ip);
            this.uiManager.updateConnectionStatus(true);
            this.startSensorUpdates();
        } catch (error) {
            console.error('Connection failed:', error);
            alert('Failed to connect to robot. Please check the IP address and try again.');
            this.uiManager.updateConnectionStatus(false);
        }
    }

    updateCodeOutput() {
        const code = this.workspace.generateCode();
        document.getElementById('codeOutput').textContent = code;
    }

    async runCode() {
        if (this.isRunning) return;
        
        if (!this.robotController.isConnected()) {
            alert('Please connect to the robot first');
            return;
        }

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
        // Update sensor data every 500ms
        setInterval(() => {
            if (this.robotController.isConnected()) {
                this.uiManager.updateSensorData();
            }
        }, 500);
    }
}

// Initialize app when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    const app = new App();
    app.init();
});