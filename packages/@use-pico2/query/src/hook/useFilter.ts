import {type PicoSchema}  from "@use-pico2/schema";
import {type IQueryStore} from "../api/IQueryStore";
import {type QuerySchema} from "../schema/QuerySchema";

export namespace useFilter {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
    > {
        store: IQueryStore<TQuerySchema>;
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
    const setFilter = store.use(({setFilter}) => setFilter);
    return filter => setFilter(filter);
};
