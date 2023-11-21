"use client";

import {useQuery as useCoolQuery} from "@tanstack/react-query";
import {
    type IWithQuery,
    type QuerySchema
}                                 from "@use-pico/query";
import {
    type ArraySchema,
    WithIdentitySchema
}                                 from "@use-pico/schema";
import {type IQueryStore}         from "../api/IQueryStore";
import {type IWithSourceQuery}    from "../api/IWithSourceQuery";
import {usePromise}               from "./usePromise";
import {useQueryOf}               from "./useQueryOf";

export namespace useSourceQuery {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TResponseSchema extends WithIdentitySchema,
    > extends IWithQuery.QueryOptions<ArraySchema<TResponseSchema>> {
        store: IQueryStore.Store<TQuerySchema>;
        withSourceQuery: IWithSourceQuery<TQuerySchema, TResponseSchema>;
    }
}

export const useSourceQuery = <
    TQuerySchema extends QuerySchema<any, any>,
    TResponseSchema extends WithIdentitySchema,
>(
    {
        store,
        withSourceQuery,
        queryKey,
        ...options
    }: useSourceQuery.Props<TQuerySchema, TResponseSchema>
): IWithSourceQuery.Result<TResponseSchema> => {
    const promise = usePromise({withQuery: withSourceQuery});
    const query = useQueryOf({store});

    return useCoolQuery({
        queryKey: withSourceQuery.key.concat(queryKey || []).concat(query),
        queryFn:  async () => promise(query),
        ...options,
    });
};
