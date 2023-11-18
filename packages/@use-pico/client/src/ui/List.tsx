import {
    type IWithQuery,
    type QuerySchema
}                              from "@use-pico/query";
import {
    type ArraySchema,
    type PicoSchema,
    type WithIdentitySchema
}                              from "@use-pico/schema";
import {type FC}               from "react";
import {type IQueryStore}      from "../api/IQueryStore";
import {type IWithSourceQuery} from "../api/IWithSourceQuery";

export namespace List {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TResponseSchema extends WithIdentitySchema,
    > {
        withSourceQuery: IWithSourceQuery<TQuerySchema, TResponseSchema>;
        withQueryStore: IQueryStore.Store<TQuerySchema>;
        options?: IWithQuery.QueryOptions<
            ArraySchema<TResponseSchema>
        >;
        Item: Item<TResponseSchema>;
    }

    export type Item<TResponseSchema extends WithIdentitySchema> = FC<ItemProps<TResponseSchema>>;

    export interface ItemProps<TResponseSchema extends WithIdentitySchema> {
        item: PicoSchema.Output<TResponseSchema>;
    }
}

export const List = <
    TQuerySchema extends QuerySchema<any, any>,
    TResponseSchema extends WithIdentitySchema,
>(
    {
        withSourceQuery,
        withQueryStore,
        options,
        Item,
    }: List.Props<TQuerySchema, TResponseSchema>
) => {
    return "List";
};
