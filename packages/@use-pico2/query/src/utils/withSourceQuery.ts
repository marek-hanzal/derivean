import {
    type ArraySchema,
    schema,
    type WithIdentitySchema
}                              from "@use-pico2/schema";
import {type IWithQuery}       from "../api/IWithQuery";
import {type IWithSourceQuery} from "../api/IWithSourceQuery";
import {type CountSchema}      from "../schema/CountSchema";
import {type FilterSchema}     from "../schema/FilterSchema";
import {type OrderBySchema}    from "../schema/OrderBySchema";
import {type QuerySchema}      from "../schema/QuerySchema";
import {createQueryStore}      from "../store/createQueryStore";
import {withQuery}             from "./withQuery";

export namespace withSourceQuery {
    export interface Props<
        TFilterSchema extends FilterSchema,
        TOrderBySchema extends OrderBySchema,
        TQuerySchema extends QuerySchema<TFilterSchema, TOrderBySchema>,
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
    TFilterSchema extends FilterSchema,
    TOrderBySchema extends OrderBySchema,
    TQuerySchema extends QuerySchema<TFilterSchema, TOrderBySchema>,
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
        TFilterSchema,
        TOrderBySchema,
        TQuerySchema,
        TSchema
    >
): IWithSourceQuery<
    TFilterSchema,
    TOrderBySchema,
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
        schema:     {
            query,
            response: $response,
        },
        store,
        useCount:   options => {
            return withCountQuery.useQueryEx({
                request: store.use((
                    {
                        where,
                        filter,
                    }) => ({
                    where,
                    filter,
                    cursor:  undefined,
                    orderBy: undefined,
                })),
                options,
            });
        },
        useCountEx: props => {
            return withCountQuery.useQueryEx(props);
        },
        useInvalidator() {
            const invalidator = $withQuery.useInvalidator();
            const countInvalidator = withCountQuery.useInvalidator();
            return async () => {
                await invalidator();
                await countInvalidator();
            };
        },
        useFilter:         () => {
            const setFilter = store.use(({setFilter}) => setFilter);
            return filter => setFilter(filter);
        },
        useShallowFilter:  () => {
            const shallowFilter = store.use(({shallowFilter}) => shallowFilter);
            return filter => shallowFilter(filter);
        },
        useOrderBy:        () => {
            const setOrderBy = store.use(({setOrderBy}) => setOrderBy);
            return orderBy => setOrderBy(orderBy);
        },
        useShallowOrderBy: () => {
            const shallowOrderBy = store.use(({shallowOrderBy}) => shallowOrderBy);
            return orderBy => shallowOrderBy(orderBy);
        },
    };
};
