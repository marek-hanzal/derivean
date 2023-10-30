import {
    type ArraySchema,
    type PicoSchema,
    type WithIdentitySchema
}                           from "@use-pico2/schema";
import {type CountSchema}   from "../schema/CountSchema";
import {type FilterSchema}  from "../schema/FilterSchema";
import {type OrderBySchema} from "../schema/OrderBySchema";
import {type QuerySchema}   from "../schema/QuerySchema";
import {type IQueryStore}   from "./IQueryStore";
import {type IWithQuery}    from "./IWithQuery";

export interface IWithSourceQuery<
    TFilterSchema extends FilterSchema,
    TOrderBySchema extends OrderBySchema,
    TQuerySchema extends QuerySchema<TFilterSchema, TOrderBySchema>,
    TSchema extends WithIdentitySchema,
> extends Omit<
    IWithQuery<
        TQuerySchema,
        ArraySchema<TSchema>
    >,
    "schema"
> {
    schema: {
        query: TQuerySchema;
        response: ArraySchema<TSchema>;
    };

    store: IQueryStore<TQuerySchema>;

    /**
     * Get count of items provided by this query (using current query store)
     */
    useCount: IWithQuery<never, CountSchema>["useQuery"];
    /**
     * GEt count of items provided by this query using query request directly.
     */
    useCountEx: IWithQuery<TQuerySchema, CountSchema>["useQueryEx"];

    useFilter(): (filter: PicoSchema.Output<TFilterSchema>) => void;

    useShallowFilter(): (filter?: PicoSchema.Output<TFilterSchema>) => void;

    useOrderBy(): (orderBy: PicoSchema.Output<TOrderBySchema>) => void;

    useShallowOrderBy(): (orderBy?: PicoSchema.Output<TOrderBySchema>) => void;
}
