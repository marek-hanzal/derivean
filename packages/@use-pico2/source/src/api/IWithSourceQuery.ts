import {type UseQueryResult} from "@tanstack/react-query";
import {
    type CountSchema,
    type IQueryStore,
    type IWithQuery,
    type QuerySchema
}                            from "@use-pico2/query";
import {
    type ArraySchema,
    type PicoSchema,
    type WithIdentitySchema
}                            from "@use-pico2/schema";

export interface IWithSourceQuery<
    TQuerySchema extends QuerySchema<any, any>,
    TSchema extends WithIdentitySchema,
> extends IWithQuery<
    TQuerySchema,
    ArraySchema<TSchema>
> {
    store: IQueryStore<TQuerySchema>;
    withCountQuery: IWithQuery<TQuerySchema, CountSchema>;
}

export namespace IWithSourceQuery {
    export type Result<
        TResponseSchema extends WithIdentitySchema,
    > = UseQueryResult<
        PicoSchema.Output<TResponseSchema>[],
        any
    >;
}
