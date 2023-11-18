import {
    Collection,
    type IWithSourceQuery
}                                from "@use-pico/client";
import {
    type FilterSchema,
    type OrderBySchema,
    type QuerySchema
}                                from "@use-pico/query";
import {type WithIdentitySchema} from "@use-pico/schema";
import {type FC}                 from "react";

export namespace withCollection {
    export interface Props<
        TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
        TResponseSchema extends WithIdentitySchema,
    > {
        withSourceQuery: IWithSourceQuery<TQuerySchema, TResponseSchema>;
    }

    export type Query<
        TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
        TResponseSchema extends WithIdentitySchema,
    > = FC<
        Omit<
            Collection.Props<TQuerySchema, TResponseSchema>,
            "withSourceQuery"
        >
    >;
}

export const withCollection = <
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    TResponseSchema extends WithIdentitySchema,
>(
    {
        withSourceQuery,
    }: withCollection.Props<TQuerySchema, TResponseSchema>
): withCollection.Query<TQuerySchema, TResponseSchema> => {
    return props => <Collection withSourceQuery={withSourceQuery} {...props}/>;
};
