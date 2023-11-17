import {type QuerySchema} from "@use-pico/query";
import {type PicoSchema}  from "@use-pico/schema";
import {type IQueryStore} from "../api/IQueryStore";
import {useStore}         from "./useStore";

export namespace useShallowOrderBy {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
    > {
        store: IQueryStore.Store<TQuerySchema>;
    }

    export type Result<
        TQuerySchema extends QuerySchema<any, any>,
    > = (orderBy: PicoSchema.Output<TQuerySchema["shape"]["orderBy"]>) => void;
}

export const useShallowOrderBy = <
    TQuerySchema extends QuerySchema<any, any>,
>(
    {
        store,
    }: useShallowOrderBy.Props<TQuerySchema>
): useShallowOrderBy.Result<TQuerySchema> => {
    const shallowOrderBy = useStore(store, ({shallowOrderBy}) => shallowOrderBy);
    return orderBy => shallowOrderBy(orderBy);
};
