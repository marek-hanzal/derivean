import {type IContainer} from "@use-pico/container";
import {withQuery}       from "@use-pico/repository";
import {withHandler}     from "@use-pico/rpc-server";
import {
    type RequestSchema,
    type ResponseSchema
}                        from "@use-pico/schema";

export namespace withQueryHandler {
    export interface Props<
        TRequestSchema extends RequestSchema,
        TResponseSchema extends ResponseSchema,
    > {
        table: string;
        container: IContainer.Type;
        handler: {
            key: ReadonlyArray<unknown>;
            schema: {
                request: TRequestSchema;
                response: TResponseSchema;
            };
        };
    }
}

export const withQueryHandler = <
    TRequestSchema extends RequestSchema,
    TResponseSchema extends ResponseSchema,
>(
    {
        table,
        container,
        handler,
    }: withQueryHandler.Props<TRequestSchema, TResponseSchema>
) => {
    withHandler({
        container,
        key:    handler.key,
        schema: handler.schema,
        handle: async props => withQuery({
            table,
            ...props,
        })
    });
};
