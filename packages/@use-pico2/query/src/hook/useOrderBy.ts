import {type PicoSchema}  from "@use-pico2/schema";
import {useStore}         from "@use-pico2/store";
import {type IQueryStore} from "../api/IQueryStore";
import {type QuerySchema} from "../schema/QuerySchema";

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
