import {withDullSchema} from "@use-pico/dull-stuff";
import {
    filterOf,
    orderByOf
}                       from "@use-pico/query";
import {
    identityOf,
    schema
}                       from "@use-pico/schema";

export const BuildingRequirementSchema = withDullSchema({
    entity:  identityOf(z => z.object({
        buildingId: z.string,
        itemId: z.string,
        amount:     z.number,
    })),
    shape:   schema(z => z.object({
        buildingId: z.string,
        itemId: z.string,
        amount:     z.number,
    })),
    filter:  filterOf(z => z.object({
        buildingId: z.string$,
        itemId: z.string$,
    })),
    orderBy: orderByOf(["buildingId"]),
});
export type BuildingRequirementSchema = typeof BuildingRequirementSchema;
