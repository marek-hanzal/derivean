let wasm;
export function __wbg_set_wasm(val) {
    wasm = val;
}


const lTextDecoder = typeof TextDecoder === 'undefined' ? (0, module.require)('util').TextDecoder : TextDecoder;

let cachedTextDecoder = new lTextDecoder('utf-8', { ignoreBOM: true, fatal: true });

cachedTextDecoder.decode();

let cachedUint8ArrayMemory0 = null;

function getUint8ArrayMemory0() {
    if (cachedUint8ArrayMemory0 === null || cachedUint8ArrayMemory0.byteLength === 0) {
        cachedUint8ArrayMemory0 = new Uint8Array(wasm.memory.buffer);
    }
    return cachedUint8ArrayMemory0;
}

function getStringFromWasm0(ptr, len) {
    ptr = ptr >>> 0;
    return cachedTextDecoder.decode(getUint8ArrayMemory0().subarray(ptr, ptr + len));
}

let WASM_VECTOR_LEN = 0;

const lTextEncoder = typeof TextEncoder === 'undefined' ? (0, module.require)('util').TextEncoder : TextEncoder;

let cachedTextEncoder = new lTextEncoder('utf-8');

const encodeString = (typeof cachedTextEncoder.encodeInto === 'function'
    ? function (arg, view) {
    return cachedTextEncoder.encodeInto(arg, view);
}
    : function (arg, view) {
    const buf = cachedTextEncoder.encode(arg);
    view.set(buf);
    return {
        read: arg.length,
        written: buf.length
    };
});

function passStringToWasm0(arg, malloc, realloc) {

    if (realloc === undefined) {
        const buf = cachedTextEncoder.encode(arg);
        const ptr = malloc(buf.length, 1) >>> 0;
        getUint8ArrayMemory0().subarray(ptr, ptr + buf.length).set(buf);
        WASM_VECTOR_LEN = buf.length;
        return ptr;
    }

    let len = arg.length;
    let ptr = malloc(len, 1) >>> 0;

    const mem = getUint8ArrayMemory0();

    let offset = 0;

    for (; offset < len; offset++) {
        const code = arg.charCodeAt(offset);
        if (code > 0x7F) break;
        mem[ptr + offset] = code;
    }

    if (offset !== len) {
        if (offset !== 0) {
            arg = arg.slice(offset);
        }
        ptr = realloc(ptr, len, len = offset + arg.length * 3, 1) >>> 0;
        const view = getUint8ArrayMemory0().subarray(ptr + offset, ptr + len);
        const ret = encodeString(arg, view);

        offset += ret.written;
        ptr = realloc(ptr, len, offset, 1) >>> 0;
    }

    WASM_VECTOR_LEN = offset;
    return ptr;
}

let cachedDataViewMemory0 = null;

function getDataViewMemory0() {
    if (cachedDataViewMemory0 === null || cachedDataViewMemory0.buffer.detached === true || (cachedDataViewMemory0.buffer.detached === undefined && cachedDataViewMemory0.buffer !== wasm.memory.buffer)) {
        cachedDataViewMemory0 = new DataView(wasm.memory.buffer);
    }
    return cachedDataViewMemory0;
}

export function init() {
    wasm.init();
}

const NoiseFinalization = (typeof FinalizationRegistry === 'undefined')
    ? { register: () => {}, unregister: () => {} }
    : new FinalizationRegistry(ptr => wasm.__wbg_noise_free(ptr >>> 0, 1));

export class Noise {

    static __wrap(ptr) {
        ptr = ptr >>> 0;
        const obj = Object.create(Noise.prototype);
        obj.__wbg_ptr = ptr;
        NoiseFinalization.register(obj, obj.__wbg_ptr, obj);
        return obj;
    }

    __destroy_into_raw() {
        const ptr = this.__wbg_ptr;
        this.__wbg_ptr = 0;
        NoiseFinalization.unregister(this);
        return ptr;
    }

    free() {
        const ptr = this.__destroy_into_raw();
        wasm.__wbg_noise_free(ptr, 0);
    }
    /**
     * @returns {number}
     */
    get biome() {
        const ret = wasm.__wbg_get_noise_biome(this.__wbg_ptr);
        return ret;
    }
    /**
     * @param {number} arg0
     */
    set biome(arg0) {
        wasm.__wbg_set_noise_biome(this.__wbg_ptr, arg0);
    }
    /**
     * @returns {number}
     */
    get heightmap() {
        const ret = wasm.__wbg_get_noise_heightmap(this.__wbg_ptr);
        return ret;
    }
    /**
     * @param {number} arg0
     */
    set heightmap(arg0) {
        wasm.__wbg_set_noise_heightmap(this.__wbg_ptr, arg0);
    }
    /**
     * @returns {number}
     */
    get temperature() {
        const ret = wasm.__wbg_get_noise_temperature(this.__wbg_ptr);
        return ret;
    }
    /**
     * @param {number} arg0
     */
    set temperature(arg0) {
        wasm.__wbg_set_noise_temperature(this.__wbg_ptr, arg0);
    }
    /**
     * @returns {number}
     */
    get moisture() {
        const ret = wasm.__wbg_get_noise_moisture(this.__wbg_ptr);
        return ret;
    }
    /**
     * @param {number} arg0
     */
    set moisture(arg0) {
        wasm.__wbg_set_noise_moisture(this.__wbg_ptr, arg0);
    }
    /**
     * @returns {number}
     */
    get shade() {
        const ret = wasm.__wbg_get_noise_shade(this.__wbg_ptr);
        return ret;
    }
    /**
     * @param {number} arg0
     */
    set shade(arg0) {
        wasm.__wbg_set_noise_shade(this.__wbg_ptr, arg0);
    }
}

const TerrainGeneratorFinalization = (typeof FinalizationRegistry === 'undefined')
    ? { register: () => {}, unregister: () => {} }
    : new FinalizationRegistry(ptr => wasm.__wbg_terraingenerator_free(ptr >>> 0, 1));

export class TerrainGenerator {

    __destroy_into_raw() {
        const ptr = this.__wbg_ptr;
        this.__wbg_ptr = 0;
        TerrainGeneratorFinalization.unregister(this);
        return ptr;
    }

    free() {
        const ptr = this.__destroy_into_raw();
        wasm.__wbg_terraingenerator_free(ptr, 0);
    }
    /**
     * @param {string} seed
     */
    constructor(seed) {
        const ptr0 = passStringToWasm0(seed, wasm.__wbindgen_malloc, wasm.__wbindgen_realloc);
        const len0 = WASM_VECTOR_LEN;
        const ret = wasm.terraingenerator_new(ptr0, len0);
        this.__wbg_ptr = ret >>> 0;
        TerrainGeneratorFinalization.register(this, this.__wbg_ptr, this);
        return this;
    }
    /**
     * @param {number} x
     * @param {number} z
     * @returns {Noise}
     */
    coord(x, z) {
        const ret = wasm.terraingenerator_coord(this.__wbg_ptr, x, z);
        return Noise.__wrap(ret);
    }
}

export function __wbg_error_7534b8e9a36f1ab4(arg0, arg1) {
    let deferred0_0;
    let deferred0_1;
    try {
        deferred0_0 = arg0;
        deferred0_1 = arg1;
        console.error(getStringFromWasm0(arg0, arg1));
    } finally {
        wasm.__wbindgen_free(deferred0_0, deferred0_1, 1);
    }
};

export function __wbg_new_8a6f238a6ece86ea() {
    const ret = new Error();
    return ret;
};

export function __wbg_stack_0ed75d68575b0f3c(arg0, arg1) {
    const ret = arg1.stack;
    const ptr1 = passStringToWasm0(ret, wasm.__wbindgen_malloc, wasm.__wbindgen_realloc);
    const len1 = WASM_VECTOR_LEN;
    getDataViewMemory0().setInt32(arg0 + 4 * 1, len1, true);
    getDataViewMemory0().setInt32(arg0 + 4 * 0, ptr1, true);
};

export function __wbindgen_init_externref_table() {
    const table = wasm.__wbindgen_export_3;
    const offset = table.grow(4);
    table.set(0, undefined);
    table.set(offset + 0, undefined);
    table.set(offset + 1, null);
    table.set(offset + 2, true);
    table.set(offset + 3, false);
    ;
};

export function __wbindgen_throw(arg0, arg1) {
    throw new Error(getStringFromWasm0(arg0, arg1));
};

