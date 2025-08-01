import * as Blockly from 'blockly';

// Create a custom JavaScript generator for robot commands
export const javascriptGenerator = new Blockly.Generator('JavaScript');

// Define operator precedence
javascriptGenerator.ORDER_ATOMIC = 0;
javascriptGenerator.ORDER_NEW = 1.1;
javascriptGenerator.ORDER_MEMBER = 1.2;
javascriptGenerator.ORDER_FUNCTION_CALL = 2;
javascriptGenerator.ORDER_INCREMENT = 3;
javascriptGenerator.ORDER_DECREMENT = 3;
javascriptGenerator.ORDER_BITWISE_NOT = 4.1;
javascriptGenerator.ORDER_UNARY_PLUS = 4.2;
javascriptGenerator.ORDER_UNARY_MINUS = 4.3;
javascriptGenerator.ORDER_LOGICAL_NOT = 4.4;
javascriptGenerator.ORDER_TYPEOF = 4.5;
javascriptGenerator.ORDER_VOID = 4.6;
javascriptGenerator.ORDER_DELETE = 4.7;
javascriptGenerator.ORDER_AWAIT = 4.8;
javascriptGenerator.ORDER_EXPONENTIATION = 5.0;
javascriptGenerator.ORDER_MULTIPLICATION = 5.1;
javascriptGenerator.ORDER_DIVISION = 5.2;
javascriptGenerator.ORDER_MODULUS = 5.3;
javascriptGenerator.ORDER_SUBTRACTION = 6.1;
javascriptGenerator.ORDER_ADDITION = 6.2;
javascriptGenerator.ORDER_BITWISE_SHIFT = 7;
javascriptGenerator.ORDER_RELATIONAL = 8;
javascriptGenerator.ORDER_IN = 8;
javascriptGenerator.ORDER_INSTANCEOF = 8;
javascriptGenerator.ORDER_EQUALITY = 9;
javascriptGenerator.ORDER_BITWISE_AND = 10;
javascriptGenerator.ORDER_BITWISE_XOR = 11;
javascriptGenerator.ORDER_BITWISE_OR = 12;
javascriptGenerator.ORDER_LOGICAL_AND = 13;
javascriptGenerator.ORDER_LOGICAL_OR = 14;
javascriptGenerator.ORDER_CONDITIONAL = 15;
javascriptGenerator.ORDER_ASSIGNMENT = 16;
javascriptGenerator.ORDER_YIELD = 17;
javascriptGenerator.ORDER_COMMA = 18;
javascriptGenerator.ORDER_NONE = 99;

// Override the scrub_ method to handle statement formatting
javascriptGenerator.scrub_ = function(block, code, opt_thisOnly) {
    const nextBlock = block.nextConnection && block.nextConnection.targetBlock();
    if (nextBlock && !opt_thisOnly) {
        return code + javascriptGenerator.blockToCode(nextBlock);
    }
    return code;
};

// Add basic control blocks
javascriptGenerator['controls_if'] = function(block) {
    let n = 0;
    let code = '';
    
    if (javascriptGenerator.STATEMENT_PREFIX) {
        code += javascriptGenerator.injectId(javascriptGenerator.STATEMENT_PREFIX, block);
    }
    
    do {
        const conditionCode = javascriptGenerator.valueToCode(block, 'IF' + n, javascriptGenerator.ORDER_NONE) || 'false';
        const branchCode = javascriptGenerator.statementToCode(block, 'DO' + n);
        
        if (n > 0) {
            code += ' else ';
        }
        
        code += 'if (' + conditionCode + ') {\n' + branchCode + '}';
        n++;
    } while (block.getInput('IF' + n));
    
    if (block.getInput('ELSE')) {
        const elseCode = javascriptGenerator.statementToCode(block, 'ELSE');
        code += ' else {\n' + elseCode + '}';
    }
    
    return code + '\n';
};

javascriptGenerator['logic_compare'] = function(block) {
    const OPERATORS = {
        'EQ': '==',
        'NEQ': '!=',
        'LT': '<',
        'LTE': '<=',
        'GT': '>',
        'GTE': '>='
    };
    
    const operator = OPERATORS[block.getFieldValue('OP')];
    const order = (operator === '==' || operator === '!=') ? 
        javascriptGenerator.ORDER_EQUALITY : javascriptGenerator.ORDER_RELATIONAL;
    
    const argument0 = javascriptGenerator.valueToCode(block, 'A', order) || '0';
    const argument1 = javascriptGenerator.valueToCode(block, 'B', order) || '0';
    
    return [argument0 + ' ' + operator + ' ' + argument1, order];
};

javascriptGenerator['math_number'] = function(block) {
    const code = Number(block.getFieldValue('NUM'));
    const order = code >= 0 ? javascriptGenerator.ORDER_ATOMIC : javascriptGenerator.ORDER_UNARY_MINUS;
    return [code, order];
};