import {type IContainer} from "@use-pico2/container";
import {
    parse,
    type PicoSchema,
    type RequestSchema,
    type ResponseSchema
}                        from "@use-pico2/schema";

export namespace withAction {
    export interface Props<
        TRequestSchema extends RequestSchema,
        TResponseSchema extends ResponseSchema,
    > {
        (container: IContainer.Type): ActionProps<TRequestSchema, TResponseSchema>;
    }

    export interface ActionProps<
        TRequestSchema extends RequestSchema,
        TResponseSchema extends ResponseSchema,
    > {
        request: TRequestSchema;
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
    factory: withAction.Props<TRequestSchema, TResponseSchema>,
): (container: IContainer.Type) => withAction.Action<TRequestSchema, TResponseSchema> => {
    return container => {
        const {
            request,
            response,
            action
        } = factory(container);
        return async $request => parse(
            response,
            await action(
                parse(request, $request),
            )
        );
    };
};
