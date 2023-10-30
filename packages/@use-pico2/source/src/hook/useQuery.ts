import {useQuery as useCoolQuery} from "@tanstack/react-query";
import {
    type IWithQuery,
    type QuerySchema,
    usePromise
}                                 from "@use-pico2/query";
import {
    type ArraySchema,
    WithIdentitySchema
}                                 from "@use-pico2/schema";
import {type IWithSourceQuery}    from "../api/IWithSourceQuery";

export namespace useQuery {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TResponseSchema extends WithIdentitySchema,
    > extends IWithQuery.QueryOptions<ArraySchema<TResponseSchema>> {
        withSourceQuery: IWithSourceQuery<TQuerySchema, TResponseSchema>;
    }
}

export const useQuery = <
    TQuerySchema extends QuerySchema<any, any>,
    TResponseSchema extends WithIdentitySchema,
>(
    {
        withSourceQuery,
        queryKey,
        ...options
    }: useQuery.Props<TQuerySchema, TResponseSchema>
): IWithSourceQuery.Result<TResponseSchema> => {
    const promise = usePromise({withQuery: withSourceQuery});
    return useCoolQuery({
        queryKey: withSourceQuery.key.concat(queryKey || []),
        queryFn:  async () => promise({}),
        ...options,
    });
};
