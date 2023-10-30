import {type QuerySchema}      from "@use-pico2/query";
import {
    type PicoSchema,
    type WithIdentitySchema
}                              from "@use-pico2/schema";
import {type IWithSourceQuery} from "../api/IWithSourceQuery";

export namespace useShallowFilter {
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

export const useShallowFilter = <
    TQuerySchema extends QuerySchema<any, any>,
    TSchema extends WithIdentitySchema,
>(
    {
        withSourceQuery: {store},
    }: useShallowFilter.Props<TQuerySchema, TSchema>
): useShallowFilter.Result<TQuerySchema> => {
    const shallowFilter = store.use(({shallowFilter}) => shallowFilter);
    return filter => shallowFilter(filter);
};
