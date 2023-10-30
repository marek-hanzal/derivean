import {type QuerySchema}      from "@use-pico2/query";
import {
    type PicoSchema,
    type WithIdentitySchema
}                              from "@use-pico2/schema";
import {type IWithSourceQuery} from "../api/IWithSourceQuery";

export namespace useFilter {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TSchema extends WithIdentitySchema,
    > {
        withSourceQuery: IWithSourceQuery<TQuerySchema, TSchema>;
    }

    export type Result<
        TQuerySchema extends QuerySchema<any, any>,
    > = (filter: PicoSchema.Output<TQuerySchema["shape"]["filter"]>) => void;
}

export const useFilter = <
    TQuerySchema extends QuerySchema<any, any>,
    TSchema extends WithIdentitySchema,
>(
    {
        withSourceQuery: {store},
    }: useFilter.Props<TQuerySchema, TSchema>
): useFilter.Result<TQuerySchema> => {
    const setFilter = store.use(({setFilter}) => setFilter);
    return filter => setFilter(filter);
};
