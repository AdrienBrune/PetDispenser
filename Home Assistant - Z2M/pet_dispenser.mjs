import * as exposes from 'zigbee-herdsman-converters/lib/exposes';

const e = exposes.presets;
const ea = exposes.access;

const CUSTOM_CLUSTER_ID = 65280; // 0xFF00 (pour dispense_food)
const ANALOG_INPUT_CLUSTER = 'genAnalogInput'; // Pour tank_filling
const ANALOG_VALUE_CLUSTER = 'genAnalogValue'; // Pour portion_weight et portion_per_turn
const MULTISTATE_VALUE_CLUSTER = 'genMultistateValue'; // Pour motor_speed
const BINARY_VALUE_CLUSTER = 'genBinaryValue'; // Pour sleep_mode

// Table de correspondance pour le moteur
const SPEED_MAP = { slow: 0, medium: 1, fast: 2 };
const SPEED_MAP_INV = { 0: 'slow', 1: 'medium', 2: 'fast' };

export default {
    zigbeeModel: ['PET-DISPENSER'],
    model: 'PET-DISPENSER',
    vendor: 'Custom',
    description: 'Distributeur de croquettes DIY',
    icon: 'device_icons/pet_dispenser.png',
    fromZigbee: [
        {
            cluster: CUSTOM_CLUSTER_ID,
            type: ['attributeReport', 'readResponse', 'commandReceive', 'raw'],
            convert: (model, msg, publish, options, meta) => {
                const result = {};
                const data = msg.data || {};
                if (data['0x0000'] !== undefined || data[0] !== undefined) {
                    const val = data['0x0000'] !== undefined ? data['0x0000'] : data[0];
                    result.dispense_food = val ? 'PRESS' : 'IDLE';
                }
                return result;
            },
        },
        {
            cluster: ANALOG_INPUT_CLUSTER,
            type: ['attributeReport', 'readResponse'],
            convert: (model, msg, publish, options, meta) => {
                const result = {};
                if (msg.data['presentValue'] !== undefined) {
                    result.tank_filling = msg.data['presentValue'];
                }
                return result;
            },
        },
        {
            cluster: ANALOG_VALUE_CLUSTER,
            type: ['attributeReport', 'readResponse'],
            convert: (model, msg, publish, options, meta) => {
                const result = {};
                if (msg.data['presentValue'] !== undefined) {
                    const val = Number(msg.data['presentValue'].toFixed(1));
                    if (msg.endpoint && msg.endpoint.ID === 2) {
                        result.portion_per_turn = val;
                    } else {
                        result.portion_weight = val;
                    }
                }
                return result;
            },
        },
        {
            cluster: MULTISTATE_VALUE_CLUSTER,
            type: ['attributeReport', 'readResponse'],
            convert: (model, msg, publish, options, meta) => {
                const result = {};
                if (msg.data['presentValue'] !== undefined) {
                    const val = msg.data['presentValue'];
                    if (SPEED_MAP_INV[val] !== undefined) {
                        result.motor_speed = SPEED_MAP_INV[val];
                    }
                }
                return result;
            },
        },
        {
            cluster: BINARY_VALUE_CLUSTER,
            type: ['attributeReport', 'readResponse'],
            convert: (model, msg, publish, options, meta) => {
                const result = {};
                if (msg.data['presentValue'] !== undefined) {
                    result.sleep_mode = msg.data['presentValue'] ? true : false;
                }
                return result;
            },
        },
    ],
    toZigbee: [
        {
            key: ['tank_filling'],
            convertSet: async (entity, key, value, meta) => {
                const endpoint = meta.device.getEndpoint(1);
                await endpoint.write(ANALOG_INPUT_CLUSTER, { presentValue: value });
                return { state: { [key]: value } };
            },
            convertGet: async (entity, key, meta) => {
                const endpoint = meta.device.getEndpoint(1);
                await endpoint.read(ANALOG_INPUT_CLUSTER, ['presentValue']);
            },
        },
        {
            key: ['portion_weight'],
            convertSet: async (entity, key, value, meta) => {
                const endpoint = meta.device.getEndpoint(1);
                await endpoint.write(ANALOG_VALUE_CLUSTER, { presentValue: parseFloat(value) });
                return { state: { [key]: parseFloat(value) } };
            },
            convertGet: async (entity, key, meta) => {
                const endpoint = meta.device.getEndpoint(1);
                await endpoint.read(ANALOG_VALUE_CLUSTER, ['presentValue']);
            },
        },
        {
            key: ['portion_per_turn'],
            convertSet: async (entity, key, value, meta) => {
                const endpoint = meta.device.getEndpoint(2);
                await endpoint.write(ANALOG_VALUE_CLUSTER, { presentValue: parseFloat(value) });
                return { state: { [key]: parseFloat(value) } };
            },
            convertGet: async (entity, key, meta) => {
                const endpoint = meta.device.getEndpoint(2);
                await endpoint.read(ANALOG_VALUE_CLUSTER, ['presentValue']);
            },
        },
        {
            key: ['motor_speed'],
            convertSet: async (entity, key, value, meta) => {
                const endpoint = meta.device.getEndpoint(2);
                const numVal = SPEED_MAP[value] !== undefined ? SPEED_MAP[value] : 0;
                await endpoint.write(MULTISTATE_VALUE_CLUSTER, { presentValue: numVal });
                return { state: { [key]: value } };
            },
            convertGet: async (entity, key, meta) => {
                const endpoint = meta.device.getEndpoint(2);
                await endpoint.read(MULTISTATE_VALUE_CLUSTER, ['presentValue']);
            },
        },
        {
            key: ['sleep_mode', 'state'],
            convertSet: async (entity, key, value, meta) => {
                const endpoint = meta.device.getEndpoint(2);
                let boolVal = false;
                if (typeof value === 'string') {
                    boolVal = value.toUpperCase() === 'ON';
                } else {
                    boolVal = Boolean(value);
                }
                await endpoint.write(BINARY_VALUE_CLUSTER, { presentValue: boolVal });
                return { state: { sleep_mode: boolVal } };
            },
            convertGet: async (entity, key, meta) => {
                const endpoint = meta.device.getEndpoint(2);
                await endpoint.read(BINARY_VALUE_CLUSTER, ['presentValue']);
            },
        },
        {
            key: ['dispense_food'],
            convertSet: async (entity, key, value, meta) => {
                const endpoint = meta.device.getEndpoint(1);
                const payload = { 0x0000: { value: 1, type: 0x10 } };
                await endpoint.write(CUSTOM_CLUSTER_ID, payload, { manufCode: 0x0000 });
                return { state: { [key]: 'PRESS' } };
            },
            convertGet: async (entity, key, meta) => {
                const endpoint = meta.device.getEndpoint(1);
                await endpoint.read(CUSTOM_CLUSTER_ID, [0x0000], { manufCode: 0x0000 });
            },
        },
    ],
    exposes: [
        e.enum('dispense_food', ea.ALL, ['PRESS'])
            .withDescription('Trigger immediate dispensing of one portion'),
        e.numeric('portion_weight', ea.ALL)
            .withUnit('g')
            .withValueMin(1)
            .withValueMax(100)
            .withValueStep(1)
            .withDescription('Desired portion weight'),
        e.enum('motor_speed', ea.ALL, ['slow', 'medium', 'fast'])
            .withDescription('Motor speed'),
        e.numeric('portion_per_turn', ea.ALL)
            .withUnit('g')
            .withValueMin(1)
            .withValueMax(50)
            .withValueStep(1)
            .withDescription('Portion corresponding to one complete motor rotation (calibration)'),
        e.numeric('tank_filling', ea.STATE_GET)
            .withUnit('%')
            .withValueMin(0)
            .withValueMax(100)
            .withValueStep(1)
            .withDescription('Kibble tank level'),
        e.switch('sleep_mode', ea.ALL)
            .withDescription('Enable or disable motor driver sleep mode when idle'),
    ],
};