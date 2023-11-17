import {withSchema} from "@use-pico/extras";
import {
    filterOf,
    orderByOf
}                   from "@use-pico/query";
import {
    identityOf,
    schema
}                   from "@use-pico/schema";

export const TranslationSchema = withSchema({
    entity:  identityOf(z => z.object({
        locale: z.string,
        key:    z.string,
        hash:   z.string,
        value:  z.string,
    })),
    shape:   schema(z => z.object({
        locale: z.string,
        key:    z.string,
        value:  z.string,
    })),
    filter:  filterOf(z => z.object({
        locale: z.string$,
        hash:   z.string$,
        key:    z.string$,
    })),
    orderBy: orderByOf(["locale", "key"]),
});
export type TranslationSchema = typeof TranslationSchema;
