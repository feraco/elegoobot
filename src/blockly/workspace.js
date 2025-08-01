import * as Blockly from 'blockly';
import { robotBlocks } from './blocks/index.js';
import { javascriptGenerator } from './generators/javascript.js';

export class BlocklyWorkspace {
    constructor() {
        this.workspace = null;
        this.init();
    }

    init() {
        // Define the toolbox
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
                        { kind: 'block', type: 'robot_stop' },
                        { kind: 'block', type: 'robot_move_custom' },
                        { kind: 'block', type: 'robot_motor_control' }
                    ]
                },
                {
                    kind: 'category',
                    name: 'Servo',
                    colour: '#FF9800',
                    contents: [
                        { kind: 'block', type: 'robot_servo_control' },
                        { kind: 'block', type: 'robot_servo_sweep' }
                    ]
                },
                {
                    kind: 'category',
                    name: 'Lighting',
                    colour: '#9C27B0',
                    contents: [
                        { kind: 'block', type: 'robot_led_color' },
                        { kind: 'block', type: 'robot_led_off' },
                        { kind: 'block', type: 'robot_led_blink' }
                    ]
                },
                {
                    kind: 'category',
                    name: 'Sensors',
                    colour: '#2196F3',
                    contents: [
                        { kind: 'block', type: 'robot_ultrasonic_read' },
                        { kind: 'block', type: 'robot_line_tracking_read' },
                        { kind: 'block', type: 'robot_battery_read' }
                    ]
                },
                {
                    kind: 'category',
                    name: 'Control',
                    colour: '#607D8B',
                    contents: [
                        { kind: 'block', type: 'robot_wait' },
                        { kind: 'block', type: 'robot_repeat' },
                        { kind: 'block', type: 'controls_if' },
                        { kind: 'block', type: 'logic_compare' },
                        { kind: 'block', type: 'math_number' }
                    ]
                }
            ]
        };

        // Initialize workspace
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
            sounds: false,
            theme: this.createCustomTheme()
        });

        // Register custom blocks
        this.registerBlocks();
    }

    createCustomTheme() {
        return Blockly.Theme.defineTheme('robotTheme', {
            base: Blockly.Themes.Classic,
            componentStyles: {
                workspaceBackgroundColour: '#ffffff',
                toolboxBackgroundColour: '#f8fafc',
                toolboxForegroundColour: '#2d3748',
                flyoutBackgroundColour: '#ffffff',
                flyoutForegroundColour: '#2d3748',
                flyoutOpacity: 0.95,
                scrollbarColour: '#cbd5e1',
                insertionMarkerColour: '#667eea',
                insertionMarkerOpacity: 0.3
            }
        });
    }

    registerBlocks() {
        // Register all custom blocks
        Object.values(robotBlocks).forEach(blockDef => {
            Blockly.Blocks[blockDef.type] = blockDef.block;
            javascriptGenerator[blockDef.type] = blockDef.generator;
        });
    }

    generateCode() {
        return javascriptGenerator.workspaceToCode(this.workspace);
    }

    generateCommands() {
        const code = this.generateCode();
        // Parse the generated code to extract robot commands
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
        // Extract method name and parameters
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
        
        // Simple parameter parsing - handles numbers and strings
        return paramString.split(',').map(param => {
            const trimmed = param.trim();
            if (trimmed.startsWith('"') && trimmed.endsWith('"')) {
                return trimmed.slice(1, -1); // Remove quotes
            }
            const num = parseFloat(trimmed);
            return isNaN(num) ? trimmed : num;
        });
    }

    addChangeListener(callback) {
        this.workspace.addChangeListener(callback);
    }

    clear() {
        this.workspace.clear();
    }
}