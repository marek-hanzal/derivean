import {
    type PicoSchema,
    schema
} from "@use-pico/schema";

export const ResourceShapeSchema = schema(z => z.object({
    name:   z.string,
    typeId: z.string,
}));
export type ResourceShapeSchema = typeof ResourceShapeSchema;
export namespace ResourceShapeSchema {
    export type Type = PicoSchema.Output<ResourceShapeSchema>;
}
