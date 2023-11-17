import {type QuerySchema} from "@use-pico/query";
import {type PicoSchema}  from "@use-pico/schema";
import {type IQueryStore} from "../api/IQueryStore";
import {useStore}         from "./useStore";

export namespace useOrderBy {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
    > {
        store: IQueryStore.Store<TQuerySchema>;
    }

    export type Result<
        TQuerySchema extends QuerySchema<any, any>,
    > = (orderBy: PicoSchema.Output<TQuerySchema["shape"]["orderBy"]>) => void;
}

export const useOrderBy = <
    TQuerySchema extends QuerySchema<any, any>,
>(
    {
        store,
    }: useOrderBy.Props<TQuerySchema>
): useOrderBy.Result<TQuerySchema> => {
    const setOrderBy = useStore(store, ({setOrderBy}) => setOrderBy);
    return orderBy => setOrderBy(orderBy);
};
