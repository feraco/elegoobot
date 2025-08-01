export const sensorBlocks = {
    robot_ultrasonic_read: {
        type: 'robot_ultrasonic_read',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("ultrasonic distance (cm)");
                this.setOutput(true, "Number");
                this.setColour('#2196F3');
                this.setTooltip("Read distance from ultrasonic sensor in centimeters");
            }
        },
        generator: function(block) {
            return ['robot.readUltrasonic()', Blockly.JavaScript.ORDER_FUNCTION_CALL];
        }
    },

    robot_line_tracking_read: {
        type: 'robot_line_tracking_read',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("line tracking sensor")
                    .appendField(new Blockly.FieldDropdown([
                        ["left", "L"],
                        ["middle", "M"],
                        ["right", "R"]
                    ]), "SENSOR");
                this.setOutput(true, "Number");
                this.setColour('#2196F3');
                this.setTooltip("Read value from line tracking sensor");
            }
        },
        generator: function(block) {
            const sensor = block.getFieldValue('SENSOR');
            return [`robot.readLineTracking("${sensor}")`, Blockly.JavaScript.ORDER_FUNCTION_CALL];
        }
    },

    robot_battery_read: {
        type: 'robot_battery_read',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("battery voltage (V)");
                this.setOutput(true, "Number");
                this.setColour('#2196F3');
                this.setTooltip("Read battery voltage in volts");
            }
        },
        generator: function(block) {
            return ['robot.readBattery()', Blockly.JavaScript.ORDER_FUNCTION_CALL];
        }
    }
};