import {
    useMutation as useCoolMutation,
    useQueryClient
}                           from "@tanstack/react-query";
import {
    parse,
    type RequestSchema,
    type ResponseSchema
}                           from "@use-pico2/schema";
import {type IWithMutation} from "../api/IWithMutation";

export namespace useMutation {
    export interface Props<
        TRequestSchema extends RequestSchema,
        TResponseSchema extends ResponseSchema,
    > extends IWithMutation.Options<TRequestSchema, TResponseSchema> {
        withMutation: IWithMutation<TRequestSchema, TResponseSchema>;
    }
}

export const useMutation = <
    TRequestSchema extends RequestSchema,
    TResponseSchema extends ResponseSchema,
>(
    {
        withMutation: {
                          key,
                          mutator,
                          schema,
                          invalidator,
                          defaultOptions: {
                                              onSuccess: onDefaultSuccess,
                                              ...        defaultOptions
                                          } = {},
                      },
        mutationKey:  $mutationKey,
        onSuccess,
        ...options
    }: useMutation.Props<TRequestSchema, TResponseSchema>
) => {
    const queryClient = useQueryClient();
    return useCoolMutation({
        mutationKey: key.concat($mutationKey || []),
        mutationFn:  request => {
            try {
                return parse(
                    schema.response,
                    mutator(
                        parse(schema.request, request)
                    )
                );
            } catch (e) {
                console.error(e);
                throw e;
            }
        },
        ...defaultOptions,
        ...options,
        onSuccess: async (data, variables, context) => {
            setTimeout(() => {
                invalidator?.({
                    queryClient,
                });
            }, 0);
            onDefaultSuccess?.(data, variables, context);
            onSuccess?.(data, variables, context);
        },
    });
};
