/**
 * This is a simple and fast string check function.
 *
 * It's intended to be used on simple string checks; it does not handle valueOf or other
 * magical string objects.
 */
export const isString = (val: unknown): val is string => typeof val === "string";
