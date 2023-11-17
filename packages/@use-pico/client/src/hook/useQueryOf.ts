import {type QuerySchema} from "@use-pico/query";
import {type IQueryStore} from "../api/IQueryStore";
import {useStore}         from "./useStore";

export namespace useQueryStore {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
    > {
        store: IQueryStore.Store<TQuerySchema>;
    }
}
/**
 * Direct access to a query of the provided source.
 */
export const useQueryOf = <
    TQuerySchema extends QuerySchema<any, any>,
>(
    {
        store
    }: useQueryStore.Props<TQuerySchema>
) => {
    return useStore(store, (
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
