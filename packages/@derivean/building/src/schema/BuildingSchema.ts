import {
    identityOf,
    type PicoSchema
} from "@use-pico/schema";

export const BuildingSchema = identityOf(z => z.object({
    name: z.string,
}));
export type BuildingSchema = typeof BuildingSchema;
export namespace BuildingSchema {
    export type Type = PicoSchema.Output<BuildingSchema>;
}
