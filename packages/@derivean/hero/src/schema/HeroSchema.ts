import {withSchema} from "@use-pico/extras";
import {
    filterOf,
    orderByOf
}                   from "@use-pico/query";
import {
    identityOf,
    schema
}                   from "@use-pico/schema";

export const HeroSchema = withSchema({
    entity:  identityOf(z => z.object({
        userId:      z.string,
        kingdomId:   z.string,
        inventoryId: z.string,
        name:        z.string,
        health:      z.number,
        level:       z.number,
        prestige:    z.number,
    })),
    shape:   schema(z => z.object({
        userId:      z.string$,
        kingdomId:   z.string,
        inventoryId: z.string$,
        name:        z.string,
        health:      z.number,
        level:       z.number,
        prestige:    z.number,
    })),
    filter:  filterOf(z => z.object({
        userId:    z.string$,
        kingdomId: z.string$,
    })),
    orderBy: orderByOf(["name", "prestige", "level"]),
});
export type HeroSchema = typeof HeroSchema;
