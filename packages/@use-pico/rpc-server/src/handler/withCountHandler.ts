import {type IContainer}    from "@use-pico/container";
import {CountSchema}        from "@use-pico/query";
import {
    type IWithWhere,
    withCount
}                           from "@use-pico/repository";
import {type RequestSchema} from "@use-pico/schema";
import {withHandler}        from "../rpc/withHandler";

export namespace withCountHandler {
    export interface Props<
        TRequestSchema extends RequestSchema,
    > {
        table: string;
        container: IContainer.Type;
        handler: {
            key: ReadonlyArray<unknown>;
            schema: {
                request: TRequestSchema;
                response: CountSchema;
            };
        };
        withFilter?: IWithWhere<any, any>;
        withWhere?: IWithWhere<any, any>;
    }
}

export const withCountHandler = <
    TRequestSchema extends RequestSchema,
>(
    {
        table,
        container,
        handler,
        withWhere,
        withFilter,
    }: withCountHandler.Props<TRequestSchema>
) => withHandler({
    container,
    key:    handler.key,
    schema: handler.schema,
    handle: async props => withCount({
        table,
        withWhere,
        withFilter,
        ...props,
    })
});
