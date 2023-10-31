import {type IContainer}     from "@use-pico/container";
import {type QuerySchema}    from "@use-pico/query";
import {
    type IWithWhere,
    withQuery
}                            from "@use-pico/repository";
import {type ResponseSchema} from "@use-pico/schema";
import {withHandler}         from "../rpc/withHandler";

export namespace withQueryHandler {
    export interface Props<
        TRequestSchema extends QuerySchema<any, any>,
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
        withFilter?: IWithWhere<any, any>;
        withWhere?: IWithWhere<any, any>;
    }
}

export const withQueryHandler = <
    TRequestSchema extends QuerySchema<any, any>,
    TResponseSchema extends ResponseSchema,
>(
    {
        table,
        container,
        handler,
        withWhere,
        withFilter
    }: withQueryHandler.Props<TRequestSchema, TResponseSchema>
) => withHandler({
    container,
    key:    handler.key,
    schema: handler.schema,
    handle: async props => withQuery({
        table,
        withWhere,
        withFilter,
        ...props,
    })
});
