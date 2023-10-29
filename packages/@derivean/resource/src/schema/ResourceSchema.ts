import {
    identityOf,
    type PicoSchema
} from "@use-pico2/schema";

export const ResourceSchema = identityOf(z => z.object({
    name: z.string,
}));
export type ResourceSchema = typeof ResourceSchema;
export namespace ResourceSchema {
    export type Type = PicoSchema.Output<ResourceSchema>;
}
