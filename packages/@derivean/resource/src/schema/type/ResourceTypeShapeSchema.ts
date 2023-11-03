import {
    type PicoSchema,
    schema
} from "@use-pico/schema";

export const ResourceTypeShapeSchema = schema(z => z.object({
    name: z.string,
}));
export type ResourceTypeShapeSchema = typeof ResourceTypeShapeSchema;
export namespace ResourceTypeShapeSchema {
    export type Type = PicoSchema.Output<ResourceTypeShapeSchema>;
}
