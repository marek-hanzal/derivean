"use client";

import {
    type FilterSchema,
    type IWithQuery,
    type IWithSourceQuery,
    type OrderBySchema,
    QueryResult,
    type QuerySchema
}               from "@use-pico2/query";
import {
    type ArraySchema,
    type PicoSchema,
    type ResponseSchema,
    WithIdentitySchema
}               from "@use-pico2/schema";
import {Loader} from "@use-pico2/ui";
import {
    type FC,
    type ReactNode
}               from "react";

export namespace QueryFetch {
    export interface Props<
        TFilterSchema extends FilterSchema,
        TOrderBySchema extends OrderBySchema,
        TQuerySchema extends QuerySchema<TFilterSchema, TOrderBySchema>,
        TResponseSchema extends WithIdentitySchema,
    > {
        loader?: ReactNode;
        /**
         * Query to fetch entity
         */
        withSourceQuery: IWithSourceQuery<TFilterSchema, TOrderBySchema, TQuerySchema, TResponseSchema>;
        query: PicoSchema.Output<TQuerySchema>;

        /**
         * Error renderer
         */
        WithError?: FC<WithErrorProps>;

        /**
         * Success renderer
         */
        WithSuccess: FC<WithSuccessProps<TResponseSchema>>;
        enabled?: boolean;
        options?: IWithQuery.QueryOptions<TQuerySchema, ArraySchema<TResponseSchema>>;
    }

    export interface WithErrorProps {
        error: any;
    }

    export interface WithSuccessProps<TResponseSchema extends ResponseSchema> {
        entities: PicoSchema.Output<TResponseSchema>[];
    }
}

export const QueryFetch = <
    TFilterSchema extends FilterSchema,
    TOrderBySchema extends OrderBySchema,
    TQuerySchema extends QuerySchema<TFilterSchema, TOrderBySchema>,
    TResponseSchema extends WithIdentitySchema,
>(
    {
        loader,
        withSourceQuery,
        query,
        WithError = () => null,
        WithSuccess,
        enabled = true,
        options,
    }: QueryFetch.Props<
        TFilterSchema,
        TOrderBySchema,
        TQuerySchema,
        TResponseSchema
    >
) => {
    const result = withSourceQuery.useQueryEx({
        request: query,
        options: {
            ...options,
            enabled,
        },
    });
    return <QueryResult
        result={result}
        WithLoading={() => loader || <Loader type={"dots"} size={"xs"}/>}
        WithError={WithError}
        WithSuccess={({data}) => <WithSuccess entities={data}/>}
    />;
};
