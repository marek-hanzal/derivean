import {type QuerySchema}      from "@use-pico2/query";
import {
    type PicoSchema,
    type WithIdentitySchema
}                              from "@use-pico2/schema";
import {type IWithSourceQuery} from "../api/IWithSourceQuery";

export namespace useShallowOrderBy {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TSchema extends WithIdentitySchema,
    > {
        withSourceQuery: IWithSourceQuery<TQuerySchema, TSchema>;
    }

    export type Result<
        TQuerySchema extends QuerySchema<any, any>,
    > = (orderBy: PicoSchema.Output<TQuerySchema["shape"]["orderBy"]>) => void;
}

export const useShallowOrderBy = <
    TQuerySchema extends QuerySchema<any, any>,
    TSchema extends WithIdentitySchema,
>(
    {
        withSourceQuery: {store},
    }: useShallowOrderBy.Props<TQuerySchema, TSchema>
): useShallowOrderBy.Result<TQuerySchema> => {
    const shallowOrderBy = store.use(({shallowOrderBy}) => shallowOrderBy);
    return orderBy => shallowOrderBy(orderBy);
};
