import pupa from "pupa";

export const interpolate = (input: string, values?: Record<string, any>): string => {
    console.log("Interpolate", input, "with", values);
    return pupa(input, values || {}, {
        ignoreMissing: true,
    });
};
