export const movementBlocks = {
    robot_move_forward: {
        type: 'robot_move_forward',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("move forward");
                this.appendValueInput("SPEED")
                    .setCheck("Number")
                    .appendField("speed");
                this.appendValueInput("TIME")
                    .setCheck("Number")
                    .appendField("for")
                    .appendField(new Blockly.field.Number(1, 0.1, 10, 0.1), "TIME")
                    .appendField("seconds");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#4CAF50');
                this.setTooltip("Move the robot forward at specified speed for given time");
            }
        },
        generator: function(block) {
            const speed = Blockly.JavaScript.valueToCode(block, 'SPEED', Blockly.JavaScript.ORDER_ATOMIC) || '200';
            const time = block.getFieldValue('TIME');
            return `robot.moveForward(${speed}, ${time});\n`;
        }
    },

    robot_move_backward: {
        type: 'robot_move_backward',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("move backward");
                this.appendValueInput("SPEED")
                    .setCheck("Number")
                    .appendField("speed");
                this.appendValueInput("TIME")
                    .setCheck("Number")
                    .appendField("for")
                    .appendField(new Blockly.field.Number(1, 0.1, 10, 0.1), "TIME")
                    .appendField("seconds");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#4CAF50');
                this.setTooltip("Move the robot backward at specified speed for given time");
            }
        },
        generator: function(block) {
            const speed = Blockly.JavaScript.valueToCode(block, 'SPEED', Blockly.JavaScript.ORDER_ATOMIC) || '200';
            const time = block.getFieldValue('TIME');
            return `robot.moveBackward(${speed}, ${time});\n`;
        }
    },

    robot_turn_left: {
        type: 'robot_turn_left',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("turn left for")
                    .appendField(new Blockly.field.Number(1, 0.1, 10, 0.1), "TIME")
                    .appendField("seconds");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#4CAF50');
                this.setTooltip("Turn the robot left for specified time");
            }
        },
        generator: function(block) {
            const time = block.getFieldValue('TIME');
            return `robot.turnLeft(${time});\n`;
        }
    },

    robot_turn_right: {
        type: 'robot_turn_right',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("turn right for")
                    .appendField(new Blockly.field.Number(1, 0.1, 10, 0.1), "TIME")
                    .appendField("seconds");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#4CAF50');
                this.setTooltip("Turn the robot right for specified time");
            }
        },
        generator: function(block) {
            const time = block.getFieldValue('TIME');
            return `robot.turnRight(${time});\n`;
        }
    },

    robot_stop: {
        type: 'robot_stop',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("stop robot");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#4CAF50');
                this.setTooltip("Stop all robot movement");
            }
        },
        generator: function(block) {
            return `robot.stop();\n`;
        }
    },

    robot_move_custom: {
        type: 'robot_move_custom',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("move")
                    .appendField(new Blockly.field.Dropdown([
                        ["forward", "1"],
                        ["backward", "2"],
                        ["left", "3"],
                        ["right", "4"],
                        ["stop", "100"]
                    ]), "DIRECTION");
                this.appendValueInput("SPEED")
                    .setCheck("Number")
                    .appendField("speed");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#4CAF50');
                this.setTooltip("Move robot in specified direction with custom speed");
            }
        },
        generator: function(block) {
            const direction = block.getFieldValue('DIRECTION');
            const speed = Blockly.JavaScript.valueToCode(block, 'SPEED', Blockly.JavaScript.ORDER_ATOMIC) || '200';
            return `robot.moveCustom(${direction}, ${speed});\n`;
        }
    },

    robot_motor_control: {
        type: 'robot_motor_control',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("control motor")
                    .appendField(new Blockly.field.Dropdown([
                        ["left", "1"],
                        ["right", "2"],
                        ["both", "3"]
                    ]), "MOTOR");
                this.appendDummyInput()
                    .appendField("direction")
                    .appendField(new Blockly.field.Dropdown([
                        ["forward", "1"],
                        ["backward", "2"],
                        ["stop", "3"]
                    ]), "DIRECTION");
                this.appendValueInput("SPEED")
                    .setCheck("Number")
                    .appendField("speed");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#4CAF50');
                this.setTooltip("Control individual motors with direction and speed");
            }
        },
        generator: function(block) {
            const motor = block.getFieldValue('MOTOR');
            const direction = block.getFieldValue('DIRECTION');
            const speed = Blockly.JavaScript.valueToCode(block, 'SPEED', Blockly.JavaScript.ORDER_ATOMIC) || '200';
            return `robot.controlMotor(${motor}, ${direction}, ${speed});\n`;
        }
    }
};