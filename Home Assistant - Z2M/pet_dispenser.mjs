import * as exposes from 'zigbee-herdsman-converters/lib/exposes';

const e = exposes.presets;
const ea = exposes.access;

const CUSTOM_CLUSTER_ID = 65280; // 0xFF00 (pour dispense_food)
const ANALOG_INPUT_CLUSTER = 'genAnalogInput'; // Pour tank_filling
const ANALOG_VALUE_CLUSTER = 'genAnalogValue'; // Pour food_portion_size
const MULTISTATE_VALUE_CLUSTER = 'genMultistateValue'; // Pour motor_speed (Cluster 20 / 0x0014)

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
        // 1. Cluster propriétaire (uniquement pour dispense_food)
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
        // 2. Cluster Analog Input (tank_filling)
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
        // 3. Cluster Analog Value (food_portion_size)
        {
            cluster: ANALOG_VALUE_CLUSTER,
            type: ['attributeReport', 'readResponse'],
            convert: (model, msg, publish, options, meta) => {
                const result = {};
                if (msg.data['presentValue'] !== undefined) {
                    result.food_portion_size = Number(msg.data['presentValue'].toFixed(1));
                }
                return result;
            },
        },
        // 4. Cluster Multistate Value (motor_speed)
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
    ],
    toZigbee: [
        {
            key: ['dispense_food', 'motor_speed', 'food_portion_size', 'tank_filling'],
            convertSet: async (entity, key, value, meta) => {
                if (key === 'tank_filling') {
                    const payload = { presentValue: value };
                    await entity.write(ANALOG_INPUT_CLUSTER, payload);
                    return { state: { [key]: value } };
                } 
                else if (key === 'food_portion_size') {
                    const payload = { presentValue: parseFloat(value) };
                    await entity.write(ANALOG_VALUE_CLUSTER, payload);
                    return { state: { [key]: parseFloat(value) } };
                }
                else if (key === 'motor_speed') {
                    const numVal = SPEED_MAP[value] !== undefined ? SPEED_MAP[value] : 0;
                    const payload = { presentValue: numVal };
                    await entity.write(MULTISTATE_VALUE_CLUSTER, payload);
                    return { state: { [key]: value } };
                }
                else {
                    // Écriture sur le cluster propriétaire (dispense_food)
                    const payload = {
                        0x0000: {
                            value: 1,
                            type: 0x10 // Boolean
                        }
                    };
                    await entity.write(CUSTOM_CLUSTER_ID, payload, { manufCode: 0x0000 });
                    return { state: { [key]: 'PRESS' } };
                }
            },
            convertGet: async (entity, key, meta) => {
                if (key === 'tank_filling') {
                    await entity.read(ANALOG_INPUT_CLUSTER, ['presentValue']);
                } 
                else if (key === 'food_portion_size') {
                    await entity.read(ANALOG_VALUE_CLUSTER, ['presentValue']);
                }
                else if (key === 'motor_speed') {
                    await entity.read(MULTISTATE_VALUE_CLUSTER, ['presentValue']);
                }
                else {
                    await entity.read(CUSTOM_CLUSTER_ID, [0x0000], { manufCode: 0x0000 });
                }
            },
        },
    ],
    exposes: [
        e.enum('dispense_food', ea.ALL, ['PRESS'])
            .withDescription('Déclencher la distribution immédiate d\'une portion'),
        e.numeric('food_portion_size', ea.ALL)
            .withUnit('tours')
            .withDescription('Nombre de rotations du moteur lors d\'une distribution (1 tour = 10g)'),
        e.enum('motor_speed', ea.ALL, ['slow', 'medium', 'fast'])
            .withDescription('Vitesse du moteur'),
        e.numeric('tank_filling', ea.STATE_GET)
            .withUnit('%')
            .withDescription('Niveau du réservoir de croquettes'),
    ],
};