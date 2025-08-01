export const servoBlocks = {
    robot_servo_control: {
        type: 'robot_servo_control',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("move servo")
                    .appendField(new Blockly.FieldDropdown([
                        ["horizontal", "1"],
                        ["vertical", "2"],
                        ["both", "3"]
                    ]), "SERVO");
                this.appendDummyInput()
                    .appendField("to angle")
                    .appendField(new Blockly.FieldNumber(90, 0, 180, 1), "ANGLE");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#FF9800');
                this.setTooltip("Control servo motor position (0-180 degrees)");
            }
        },
        generator: function(block) {
            const servo = block.getFieldValue('SERVO');
            const angle = block.getFieldValue('ANGLE');
            return `robot.controlServo(${servo}, ${angle});\n`;
        }
    },

    robot_servo_sweep: {
        type: 'robot_servo_sweep',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("sweep servo")
                    .appendField(new Blockly.FieldDropdown([
                        ["horizontal", "1"],
                        ["vertical", "2"]
                    ]), "SERVO");
                this.appendDummyInput()
                    .appendField("from")
                    .appendField(new Blockly.FieldNumber(0, 0, 180, 1), "START_ANGLE")
                    .appendField("to")
                    .appendField(new Blockly.FieldNumber(180, 0, 180, 1), "END_ANGLE");
                this.appendDummyInput()
                    .appendField("delay")
                    .appendField(new Blockly.FieldNumber(500, 100, 2000, 100), "DELAY")
                    .appendField("ms");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#FF9800');
                this.setTooltip("Sweep servo between two angles with specified delay");
            }
        },
        generator: function(block) {
            const servo = block.getFieldValue('SERVO');
            const startAngle = block.getFieldValue('START_ANGLE');
            const endAngle = block.getFieldValue('END_ANGLE');
            const delay = block.getFieldValue('DELAY');
            return `robot.sweepServo(${servo}, ${startAngle}, ${endAngle}, ${delay});\n`;
        }
    }
};