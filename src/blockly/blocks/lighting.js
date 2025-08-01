export const lightingBlocks = {
    robot_led_color: {
        type: 'robot_led_color',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("set LED color to")
                    .appendField(new Blockly.field.Colour('#ff0000'), "COLOR");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#9C27B0');
                this.setTooltip("Set the RGB LED to a specific color");
            }
        },
        generator: function(block) {
            const color = block.getFieldValue('COLOR');
            // Convert hex color to RGB
            const r = parseInt(color.substr(1, 2), 16);
            const g = parseInt(color.substr(3, 2), 16);
            const b = parseInt(color.substr(5, 2), 16);
            return `robot.setLEDColor(${r}, ${g}, ${b});\n`;
        }
    },

    robot_led_off: {
        type: 'robot_led_off',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("turn LED off");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#9C27B0');
                this.setTooltip("Turn off the RGB LED");
            }
        },
        generator: function(block) {
            return `robot.setLEDColor(0, 0, 0);\n`;
        }
    },

    robot_led_blink: {
        type: 'robot_led_blink',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("blink LED")
                    .appendField(new Blockly.field.Colour('#ff0000'), "COLOR");
                this.appendDummyInput()
                    .appendField("times")
                    .appendField(new Blockly.field.Number(3, 1, 10, 1), "COUNT")
                    .appendField("delay")
                    .appendField(new Blockly.field.Number(500, 100, 2000, 100), "DELAY")
                    .appendField("ms");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#9C27B0');
                this.setTooltip("Blink the LED with specified color, count and delay");
            }
        },
        generator: function(block) {
            const color = block.getFieldValue('COLOR');
            const count = block.getFieldValue('COUNT');
            const delay = block.getFieldValue('DELAY');
            const r = parseInt(color.substr(1, 2), 16);
            const g = parseInt(color.substr(3, 2), 16);
            const b = parseInt(color.substr(5, 2), 16);
            return `robot.blinkLED(${r}, ${g}, ${b}, ${count}, ${delay});\n`;
        }
    }
};