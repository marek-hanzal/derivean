import {type QuerySchema}      from "@use-pico2/query";
import {
    type PicoSchema,
    type WithIdentitySchema
}                              from "@use-pico2/schema";
import {type IWithSourceQuery} from "../api/IWithSourceQuery";

export namespace useOrderBy {
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

export const useOrderBy = <
    TQuerySchema extends QuerySchema<any, any>,
    TSchema extends WithIdentitySchema,
>(
    {
        withSourceQuery: {store},
    }: useOrderBy.Props<TQuerySchema, TSchema>
): useOrderBy.Result<TQuerySchema> => {
    const setOrderBy = store.use(({setOrderBy}) => setOrderBy);
    return orderBy => setOrderBy(orderBy);
};
