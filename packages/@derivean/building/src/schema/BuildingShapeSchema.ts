import {
    type PicoSchema,
    schema
} from "@use-pico/schema";

export const BuildingShapeSchema = schema(z => z.object({
    name: z.string,
}));
export type BuildingShapeSchema = typeof BuildingShapeSchema;
export namespace BuildingShapeSchema {
    export type Type = PicoSchema.Output<BuildingShapeSchema>;
}
