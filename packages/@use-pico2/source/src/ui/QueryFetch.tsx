"use client";

import {
    ErrorResponseSchema,
    type FilterSchema,
    type OrderBySchema,
    type QuerySchema,
    type WithQuery
} from "@use-pico2/query";
import {
    type PicoSchema,
    type ResponseSchema
} from "@use-pico2/schema";
import {
    type FC,
    type ReactNode
} from "react";

export namespace QueryFetch {
    export interface Props<
        TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
        TResponseSchema extends ResponseSchema,
    > {
        loader?: ReactNode;
        filter?: PicoSchema.Output<TQuerySchema["shape"]["filter"]> | null;
        /**
         * Query to fetch entity
         */
        // withSourceQuery: WithSourceQuery<TResponseSchema, TQuerySchema>;
        // query?: ComponentProps<WithSourceQuery<TResponseSchema, TQuerySchema>["store"]["Provider"]>["defaults"];

        /**
         * Error renderer
         */
        WithError?: FC<WithErrorProps>;

        /**
         * Success renderer
         */
        WithSuccess: FC<WithSuccessProps<TResponseSchema>>;
        enabled?: boolean;
        options?: WithQuery.QueryOptions<TQuerySchema, TResponseSchema>;
    }

    export interface WithErrorProps {
        error: ErrorResponseSchema.Type;
    }

    export interface WithSuccessProps<TResponseSchema extends ResponseSchema> {
        entities: PicoSchema.Output<TResponseSchema>[];
    }
}

export const QueryFetch = <
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    TResponseSchema extends ResponseSchema,
>(
    {
        loader,
        filter,
        // withSourceQuery,
        // query,
        WithError = () => null,
        WithSuccess,
        enabled = true,
        options,
    }: QueryFetch.Props<TQuerySchema, TResponseSchema>
) => {
    return "QueryFetch";

    // const result = withSourceQuery.useQueryEx({
    //     /**
    //      * @TODO this looks strange, maybe event buggy, fix this ... Stranger Things
    //      */
    //     request: (query || filter ? {filter} : {}) as any,
    //     options: {
    //         ...options,
    //         enabled,
    //     },
    // });
    // return <QueryResult
    //     result={result}
    //     WithLoading={() => loader || <Loader type={"dots"} size={"xs"}/>}
    //     WithError={WithError}
    //     WithSuccess={({data}) => <WithSuccess entities={data}/>}
    // />;
};
