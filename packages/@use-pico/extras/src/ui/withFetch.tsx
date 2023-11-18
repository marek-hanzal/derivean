import {Fetch}   from "@use-pico/client";
import {
    type IWithQuery,
    type QuerySchema
}                from "@use-pico/query";
import {
    type ArraySchema,
    type ResponseSchema
}                from "@use-pico/schema";
import {type FC} from "react";

export namespace withFetch {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TResponseSchema extends ResponseSchema,
    > {
        withQuery: IWithQuery<TQuerySchema, ArraySchema<TResponseSchema>>;
    }

    export type Fetch<
        TQuerySchema extends QuerySchema<any, any>,
        TResponseSchema extends ResponseSchema,
    > = FC<
        Omit<
            Fetch.Props<TQuerySchema, TResponseSchema>,
            "withQuery"
        >
    >;
}

export const withFetch = <
    TQuerySchema extends QuerySchema<any, any>,
    TResponseSchema extends ResponseSchema,
>(
    {
        withQuery,
    }: withFetch.Props<
        TQuerySchema,
        TResponseSchema
    >
): withFetch.Fetch<TQuerySchema, TResponseSchema> => {
    return props => <Fetch
        withQuery={withQuery}
        {...props}
    />;
};
