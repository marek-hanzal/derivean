import {withClient}       from "@derivean/orm";
import {type IContainer}  from "@use-pico/container";
import {type QuerySchema} from "@use-pico/query";
import {
    type PicoSchema,
    type WithIdentitySchema
}                         from "@use-pico/schema";

export namespace withQuery {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TEntity extends WithIdentitySchema,
    > {
        container: IContainer.Type;
        request: PicoSchema.Output<TQuerySchema>;
        table: string;
    }
}

export const withQuery = async <
    TQuerySchema extends QuerySchema<any, any>,
    TEntity extends WithIdentitySchema,
>(
    {
        request,
        container,
        table,
    }: withQuery.Props<TQuerySchema, TEntity>
): Promise<PicoSchema.Output<TEntity>[]> => {
    const client = withClient.use(container);

    return await client.selectFrom(table as any).selectAll().execute() as any;
};
