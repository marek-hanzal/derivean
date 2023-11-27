import coolHash from "object-hash";

/**
 * Generates a hash from the given input (could be even object or function).
 */
export const hash = (value: any) => {
    return coolHash(value, {
        algorithm: "sha1",
        encoding:  "hex",
    });
};
