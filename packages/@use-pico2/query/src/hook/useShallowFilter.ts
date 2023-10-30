import {type PicoSchema}  from "@use-pico2/schema";
import {type IQueryStore} from "../api/IQueryStore";
import {type QuerySchema} from "../schema/QuerySchema";

export namespace useShallowFilter {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
    > {
        store: IQueryStore<TQuerySchema>;
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
    const shallowFilter = store.use(({shallowFilter}) => shallowFilter);
    return filter => shallowFilter(filter);
};
