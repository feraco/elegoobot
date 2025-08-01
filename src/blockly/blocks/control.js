export const controlBlocks = {
    robot_wait: {
        type: 'robot_wait',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("wait")
                    .appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME")
                    .appendField("seconds");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#607D8B');
                this.setTooltip("Wait for specified number of seconds");
            }
        },
        generator: function(block) {
            const time = block.getFieldValue('TIME');
            return `robot.wait(${time});\n`;
        }
    },

    robot_repeat: {
        type: 'robot_repeat',
        block: {
            init: function() {
                this.appendDummyInput()
                    .appendField("repeat")
                    .appendField(new Blockly.FieldNumber(3, 1, 100, 1), "TIMES")
                    .appendField("times");
                this.appendStatementInput("DO")
                    .setCheck(null)
                    .appendField("do");
                this.setPreviousStatement(true, null);
                this.setNextStatement(true, null);
                this.setColour('#607D8B');
                this.setTooltip("Repeat the enclosed blocks specified number of times");
            }
        },
        generator: function(block) {
            const times = block.getFieldValue('TIMES');
            const statements = Blockly.JavaScript.statementToCode(block, 'DO');
            return `for (let i = 0; i < ${times}; i++) {\n${statements}}\n`;
        }
    }
};