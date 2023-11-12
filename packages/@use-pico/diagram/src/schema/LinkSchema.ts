import {
    type PicoSchema,
    schema
} from "@use-pico/schema";

export const LinkSchema = schema(z => z.object({
    input:  z.nonEmptyString,
    output: z.nonEmptyString,
}));
export type LinkSchema = typeof LinkSchema;
export namespace LinkSchema {
    export type Type = PicoSchema.Output<LinkSchema>;
}
