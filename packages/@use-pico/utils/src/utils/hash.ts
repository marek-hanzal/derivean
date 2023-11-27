import coolHash from "object-hash";

/**
 * Generates a hash from the given input (could be even object or function).
 *
 * Useful when you want to listen on "useEffect" with an object as a dependency. Be careful as this method
 * could be quite slow, even it's somehow optimized.
 */
export const hash = (value: any) => {
    try {
        return JSON.stringify(value);
    } catch (e) {
        return coolHash(value, {
            algorithm:                 "sha1",
            encoding:                  "hex",
            ignoreUnknown:             true,
            respectType:               false,
            respectFunctionProperties: false,
            unorderedArrays:           true,
        });
    }
};
