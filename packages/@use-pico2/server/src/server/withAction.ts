import {
    type PicoSchema,
    type RequestSchema,
    type ResponseSchema
} from "@use-pico2/schema";

export namespace withAction {
    export interface Props<
        TRequestSchema extends RequestSchema,
        TResponseSchema extends ResponseSchema,
    > {
        request: TResponseSchema;
        response: TResponseSchema;
        action: Action<TRequestSchema, TResponseSchema>;
    }

    export type Action<
        TRequestSchema extends RequestSchema,
        TResponseSchema extends ResponseSchema,
    > = (request: PicoSchema.Output<TRequestSchema>) => Promise<PicoSchema.Output<TResponseSchema>>;
}

export const withAction = <
    TRequestSchema extends RequestSchema,
    TResponseSchema extends ResponseSchema,
>(
    {
        request,
        response,
        action,
    }: withAction.Props<TRequestSchema, TResponseSchema>,
): withAction.Action<TRequestSchema, TResponseSchema> => {
    return async request => action(request);
};
