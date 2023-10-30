"use client";

import {
    type QueryKey,
    useQuery,
    useQueryClient
}                          from "@tanstack/react-query";
import {
    parse,
    type PicoSchema,
    type RequestSchema,
    type ResponseSchema
}                          from "@use-pico2/schema";
import {type IInvalidator} from "../api/IInvalidator";
import {type IWithQuery}   from "../api/IWithQuery";

export namespace withQuery {
    export interface Props<
        TRequestSchema extends RequestSchema,
        TResponseSchema extends ResponseSchema,
    > {
        key: string[];
        schema: {
            request: TRequestSchema;
            response: TResponseSchema;
        },
        invalidator?: IInvalidator.Invalidator;

        callback(request: PicoSchema.Output<TRequestSchema>): Promise<PicoSchema.Output<TResponseSchema>>;
    }
}

/**
 * Creates useQuery hook (basically same as the one in React Query).
 */
export const withQuery = <
    TRequestSchema extends RequestSchema,
    TResponseSchema extends ResponseSchema,
>(
    {
        key,
        schema: {
                    request:  requestSchema,
                    response: responseSchema,
                },
        invalidator,
        callback,
    }: withQuery.Props<TRequestSchema, TResponseSchema>
): IWithQuery<TRequestSchema, TResponseSchema> => {
    const queryKey: QueryKey = key;

    const usePromise = () => {
        return async (request: PicoSchema.Output<TRequestSchema>) => {
            return callback(requestSchema ? parse(requestSchema, request) : request);
        };
    };

    return {
        key:    queryKey,
        schema: {
            request:  requestSchema,
            response: responseSchema,
        },
        useInvalidator() {
            const queryClient = useQueryClient();
            return invalidator ? (async () => {
                return invalidator({
                    queryClient,
                });
            }) : (async () => {
                return queryClient.invalidateQueries({
                    queryKey,
                });
            });
        },
        usePromise,
        useQuery:      ({
                            queryKey: $queryKey,
                            ...       options
                        } = {}) => {
            const promise = usePromise();
            return useQuery({
                queryKey: queryKey.concat($queryKey || []),
                queryFn:  async () => promise({}),
                ...options,
            });
        },
        useQueryEx:    (
                           {
                               options,
                               request,
                           }: IWithQuery.Options<TRequestSchema, TResponseSchema>
                       ) => {
            const promise = usePromise();
            return useQuery({
                queryKey: queryKey.concat(request),
                queryFn:  async () => promise(request),
                ...options,
            });
        },
        useUpdateWith: () => {
            const queryClient = useQueryClient();
            return request => {
                queryClient.setQueryData(queryKey.concat(request ? [request] : []), request);
            };
        }
    };
};
