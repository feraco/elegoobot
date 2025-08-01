import { movementBlocks } from './movement.js';
import { servoBlocks } from './servo.js';
import { lightingBlocks } from './lighting.js';
import { sensorBlocks } from './sensors.js';
import { controlBlocks } from './control.js';

export const robotBlocks = {
    ...movementBlocks,
    ...servoBlocks,
    ...lightingBlocks,
    ...sensorBlocks,
    ...controlBlocks
};