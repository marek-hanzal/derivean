"use client";

import {
    type IWithQuery,
    QueryResult,
    type QuerySchema
}                              from "@use-pico2/query";
import {
    type ArraySchema,
    type PicoSchema,
    type ResponseSchema,
    type WithIdentitySchema
}                              from "@use-pico2/schema";
import {Loader}                from "@use-pico2/ui";
import {
    type FC,
    type ReactNode
}                              from "react";
import {type IWithSourceQuery} from "../api/IWithSourceQuery";
import {useQueryEx}            from "../hook/useQueryEx";

export namespace QueryFetch {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TResponseSchema extends WithIdentitySchema,
    > {
        loader?: ReactNode;
        /**
         * Query to fetch entity
         */
        withSourceQuery: IWithSourceQuery<TQuerySchema, TResponseSchema>;
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
        options?: IWithQuery.QueryOptions<ArraySchema<TResponseSchema>>;
    }

    export interface WithErrorProps {
        error: any;
    }

    export interface WithSuccessProps<TResponseSchema extends ResponseSchema> {
        entities: PicoSchema.Output<TResponseSchema>[];
    }
}

export const QueryFetch = <
    TQuerySchema extends QuerySchema<any, any>,
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
        TQuerySchema,
        TResponseSchema
    >
) => {
    const result = useQueryEx({
        withSourceQuery,
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