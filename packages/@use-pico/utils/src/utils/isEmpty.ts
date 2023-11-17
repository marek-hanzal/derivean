const has = Object.prototype.hasOwnProperty;
const toString = Object.prototype.toString;

/**
 * Simple function to check if a value is empty.
 */
export const isEmpty = (input: unknown) => {
    if (input == null) {
        return true;
    } else if ("boolean" == typeof input) {
        return false;
    } else if ("number" == typeof input) {
        return input === 0;
    } else if ("string" == typeof input) {
        return input.length === 0;
    } else if ("function" == typeof input) {
        return input.length === 0;
    } else if (Array.isArray(input)) {
        return input.length === 0;
    } else if (input instanceof Error) {
        return input.message === "";
    } else if (input.toString === toString) {
        switch (input.toString()) {
            case "[object File]":
            case "[object Map]":
            case "[object Set]": {
                return (input as {
                    size: number
                }).size === 0;
            }
            case "[object Object]": {
                for (const key in input) {
                    if (has.call(input, key)) {
                        return false;
                    }
                }
                return true;
            }
        }
    }
    return false;
};
