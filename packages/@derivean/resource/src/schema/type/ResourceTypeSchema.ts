import {
    identityOf,
    type PicoSchema
} from "@use-pico/schema";

export const ResourceTypeSchema = identityOf(z => z.object({
    name: z.string,
}));
export type ResourceTypeSchema = typeof ResourceTypeSchema;
export namespace ResourceTypeSchema {
    export type Type = PicoSchema.Output<ResourceTypeSchema>;
}
