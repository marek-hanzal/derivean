import {type QuerySchema} from "@use-pico/query";
import {type PicoSchema}  from "@use-pico/schema";
import {type IQueryStore} from "../api/IQueryStore";

export namespace useShallowFilter {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
    > {
        store: IQueryStore.Store<TQuerySchema>;
    }

    export type Result<
        TQuerySchema extends QuerySchema<any, any>,
    > = (filter: PicoSchema.Output<TQuerySchema["shape"]["filter"]>) => void;
}

export const useShallowFilter = <
    TQuerySchema extends QuerySchema<any, any>,
>(
    {
        store
    }: useShallowFilter.Props<TQuerySchema>
): useShallowFilter.Result<TQuerySchema> => {
    const shallowFilter = store.useSelector(({shallowFilter}) => shallowFilter);
    return filter => shallowFilter(filter);
};
