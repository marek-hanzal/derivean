import {useMutation as useCoolMutation} from "@tanstack/react-query";
import {
    parse,
    type RequestSchema,
    type ResponseSchema
}                                       from "@use-pico2/schema";
import {type IWithMutation}             from "../api/IWithMutation";
import {useInvalidator}                 from "./useInvalidator";

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
    const $invalidator = invalidator ? useInvalidator({
        invalidator: {
            invalidator,
            key
        }
    }) : (() => null);
    return useCoolMutation({
        mutationKey: key.concat($mutationKey || []),
        mutationFn: async request => {
            try {
                return parse(
                    schema.response,
                    await mutator(
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
                $invalidator();
            }, 0);
            onDefaultSuccess?.(data, variables, context);
            onSuccess?.(data, variables, context);
        },
    });
};
