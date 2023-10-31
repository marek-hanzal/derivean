import {type IContainer}  from "@use-pico/container";
import {withClient}       from "@use-pico/orm";
import {type QuerySchema} from "@use-pico/query";
import {
    type PicoSchema,
    type WithIdentitySchema
}                         from "@use-pico/schema";
import {type IWithWhere}  from "../api/IWithWhere";

export namespace withQuery {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
    > {
        container: IContainer.Type;
        request: PicoSchema.Output<TQuerySchema>;
        table: string;
        withWhere?: IWithWhere<TQuerySchema>;
        withFilter?: IWithWhere<TQuerySchema>;
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
        withWhere = ({select}) => select,
        withFilter = ({select}) => select,
    }: withQuery.Props<TQuerySchema>
): Promise<PicoSchema.Output<TEntity>[]> => {
    let query = withFilter({
        select: withWhere({
            select: withClient
                        .use(container)
                        .selectFrom(table as any),
            query:  request,
        }),
        query:  request,
    })
        .selectAll();

    request.cursor && (query = query.limit(request.cursor.size).offset(request.cursor.page * request.cursor.size));

    return await query.execute() as any;
};
