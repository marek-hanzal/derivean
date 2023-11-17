import {useQuery}        from "@tanstack/react-query";
import {type IWithQuery} from "@use-pico/query";
import {
    type RequestSchema,
    type ResponseSchema
}                        from "@use-pico/schema";
import {usePromise}      from "./usePromise";

export namespace useQueryEx {
    export interface Props<
        TRequestSchema extends RequestSchema,
        TResponseSchema extends ResponseSchema,
    > extends IWithQuery.Options<TRequestSchema, TResponseSchema> {
        withQuery: IWithQuery<TRequestSchema, TResponseSchema>;
    }
}

export const useQueryEx = <
    TRequestSchema extends RequestSchema,
    TResponseSchema extends ResponseSchema,
>(
    {
        withQuery,
        request,
        options: {
                     queryKey,
                     ...options
                 } = {}
    }: useQueryEx.Props<TRequestSchema, TResponseSchema>
) => {
    const promise = usePromise({withQuery});
    return useQuery({
        queryKey: withQuery.key.concat(queryKey || [], request),
        queryFn:  async () => promise(request),
        ...options,
    });
};
