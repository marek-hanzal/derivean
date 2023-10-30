import {type QuerySchema}        from "@use-pico2/query";
import {type WithIdentitySchema} from "@use-pico2/schema";
import {type IWithSourceQuery}   from "../api/IWithSourceQuery";

export namespace useQueryStore {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TSchema extends WithIdentitySchema,
    > {
        withSourceQuery: IWithSourceQuery<TQuerySchema, TSchema>;
    }
}
/**
 * Direct access to a query of the provided source.
 */
export const useQueryOf = <
    TQuerySchema extends QuerySchema<any, any>,
    TSchema extends WithIdentitySchema,
>(
    {
        withSourceQuery,
    }: useQueryStore.Props<TQuerySchema, TSchema>
) => {
    return withSourceQuery.store.use((
        {
            filter,
            where,
            orderBy,
            cursor
        }) => ({
        filter,
        where,
        orderBy,
        cursor
    }));
};
