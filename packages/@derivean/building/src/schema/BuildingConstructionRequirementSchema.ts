import {withSchema} from "@use-pico/extras";
import {
    filterOf,
    orderByOf
}                   from "@use-pico/query";
import {
    identityOf,
    schema
}                   from "@use-pico/schema";

export const BuildingConstructionRequirementSchema = withSchema({
    entity:  identityOf(z => z.object({
        buildingId: z.string,
        itemId:     z.string,
        amount:     z.number,
    })),
    shape:   schema(z => z.object({
        buildingId: z.string,
        itemId:     z.string,
        amount:     z.number,
    })),
    filter:  filterOf(z => z.object({
        buildingId: z.string$,
        itemId:     z.string$,
    })),
    orderBy: orderByOf(["buildingId"]),
});
export type BuildingConstructionRequirementSchema = typeof BuildingConstructionRequirementSchema;
