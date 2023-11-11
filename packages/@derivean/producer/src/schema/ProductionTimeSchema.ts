import {
    type PicoSchema,
    schema
} from "@use-pico/schema";

export const ProductionTimeSchema = schema(z => z.object({
    time: z.number,
}));
export type ProductionTimeSchema = typeof ProductionTimeSchema;
export namespace ProductionTimeSchema {
    export type Type = PicoSchema.Output<ProductionTimeSchema>;
}
