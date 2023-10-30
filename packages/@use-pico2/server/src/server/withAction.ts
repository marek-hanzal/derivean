import {
    type PicoSchema,
    type RequestSchema,
    type ResponseSchema
} from "@use-pico2/schema";

export namespace withAction {
    export type Action<
        TRequestSchema extends RequestSchema,
        TResponseSchema extends ResponseSchema,
    > = (request: PicoSchema.Output<TRequestSchema>) => Promise<PicoSchema.Output<TResponseSchema>>;
}

export const withAction = <
    TRequestSchema extends RequestSchema,
    TResponseSchema extends ResponseSchema,
>(
    handler: withAction.Action<TRequestSchema, TResponseSchema>,
): withAction.Action<TRequestSchema, TResponseSchema> => {
    return async request => handler(request);
};
