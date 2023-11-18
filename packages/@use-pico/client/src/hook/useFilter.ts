import {type QuerySchema} from "@use-pico/query";
import {type PicoSchema}  from "@use-pico/schema";
import {type IQueryStore} from "../api/IQueryStore";

export namespace useFilter {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
    > {
        store: IQueryStore.Store<TQuerySchema>;
    }

    export type Result<
        TQuerySchema extends QuerySchema<any, any>,
    > = (filter: PicoSchema.Output<TQuerySchema["shape"]["filter"]>) => void;
}

export const useFilter = <
    TQuerySchema extends QuerySchema<any, any>,
>(
    {
        store,
    }: useFilter.Props<TQuerySchema>
): useFilter.Result<TQuerySchema> => {
    const setFilter = store.useSelector(({setFilter}) => setFilter);
    return filter => setFilter(filter);
};
