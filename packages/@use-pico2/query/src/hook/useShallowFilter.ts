import {type PicoSchema}  from "@use-pico2/schema";
import {useStore}         from "@use-pico2/store";
import {type IQueryStore} from "../api/IQueryStore";
import {type QuerySchema} from "../schema/QuerySchema";

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
    const shallowFilter = useStore(store, ({shallowFilter}) => shallowFilter);
    return filter => shallowFilter(filter);
};
