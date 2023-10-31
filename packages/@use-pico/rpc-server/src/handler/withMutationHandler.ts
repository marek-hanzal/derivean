import {type IContainer}     from "@use-pico/container";
import {withMutation}        from "@use-pico/repository";
import {withHandler}         from "@use-pico/rpc-server";
import {type ResponseSchema} from "@use-pico/schema";
import {MutationSchema}      from "@use-pico/source";

export namespace withMutationHandler {
    export interface Props<
        TRequestSchema extends MutationSchema<any, any>,
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

export const withMutationHandler = <
    TRequestSchema extends MutationSchema<any, any>,
    TResponseSchema extends ResponseSchema,
>(
    {
        table,
        container,
        handler,
    }: withMutationHandler.Props<TRequestSchema, TResponseSchema>
) => {
    withHandler({
        container,
        key:    handler.key,
        schema: handler.schema,
        handle: async props => withMutation({
            table,
            ...props,
        })
    });
};
