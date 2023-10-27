import {
    type PicoSchema,
    schema
} from "@use-pico/schema";

export const ResourceSchema = schema(z => z.object({
    name: z.string,
}));

export type ResourceSchema = typeof ResourceSchema;
export namespace ResourceSchema {
    export type Type = PicoSchema.Output<ResourceSchema>;
}
