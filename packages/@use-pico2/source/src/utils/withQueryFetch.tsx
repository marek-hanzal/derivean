import {
    type FilterSchema,
    type OrderBySchema,
    type QuerySchema
}                            from "@use-pico2/query";
import {type ResponseSchema} from "@use-pico2/schema";
import {type FC}             from "react";
import {QueryFetch}          from "../ui/QueryFetch";

export namespace withQueryFetch {
    export interface Props<
        TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
        TResponseSchema extends ResponseSchema,
    > {
        // withSourceQuery: WithSourceQuery<TResponseSchema, TQuerySchema>;
    }

    export type Query<
        TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
        TResponseSchema extends ResponseSchema,
    > = FC<
        Omit<
            QueryFetch.Props<TQuerySchema, TResponseSchema>,
            "withSourceQuery"
        >
    >;
}

export const withQueryFetch = <
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    TResponseSchema extends ResponseSchema,
>(
    {
        // withSourceQuery,
    }: withQueryFetch.Props<TQuerySchema, TResponseSchema>
): withQueryFetch.Query<TQuerySchema, TResponseSchema> => {
    return () => "withQueryFetch";
    // return props => <QueryFetch
    //     withSourceQuery={withSourceQuery}
    //     {...props}
    // />;
};
