import {
    type CountSchema,
    createQueryStore,
    type IWithQuery,
    type QuerySchema,
    withQuery
}                              from "@use-pico2/query";
import {
    type ArraySchema,
    schema,
    type WithIdentitySchema
}                              from "@use-pico2/schema";
import {type IWithSourceQuery} from "../api/IWithSourceQuery";

export namespace withSourceQuery {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TSchema extends WithIdentitySchema,
    > extends Omit<
        withQuery.Props<TQuerySchema, ArraySchema<TSchema>>,
        "schema"
    > {
        schema: {
            query: TQuerySchema;
            response: TSchema;
        };
        withCountQuery: IWithQuery<TQuerySchema, CountSchema>;
    }
}

export const withSourceQuery = <
    TQuerySchema extends QuerySchema<any, any>,
    TSchema extends WithIdentitySchema,
>(
    {
        withCountQuery,
        schema: {
                    query,
                    response,
                },
        ...     props
    }: withSourceQuery.Props<
        TQuerySchema,
        TSchema
    >
): IWithSourceQuery<
    TQuerySchema,
    TSchema
> => {
    const $response = schema(z => z.array(response));
    const $withQuery = withQuery({
        ...props,
        schema: {
            request:  query,
            response: $response,
        },
    });
    const store = createQueryStore({
        name:   "QueryStore",
        schema: query,
    });

    return {
        ...$withQuery,
        schema: {
            request:  query,
            response: $response,
        },
        store,
        withCountQuery,
    };
};
