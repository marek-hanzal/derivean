"use client";

import {
    type IWithQuery,
    type QuerySchema
}                              from "@use-pico/query";
import {
    type ArraySchema,
    type PicoSchema,
    type ResponseSchema,
    type WithIdentitySchema
}                              from "@use-pico/schema";
import {
    type FC,
    type ReactNode
}                              from "react";
import {type IWithSourceQuery} from "../api/IWithSourceQuery";
import {useQueryEx}            from "../hook/useQueryEx";
import {Loader}                from "./Loader";
import {QueryResult}           from "./QueryResult";

export namespace Collection {
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

export const Collection = <
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
    }: Collection.Props<
        TQuerySchema,
        TResponseSchema
    >
) => {
    const result = useQueryEx({
        withQuery: withSourceQuery,
        request:   query,
        options:   {
            ...options,
            enabled,
        },
    });
    return <QueryResult
        result={result}
        WithLoading={() => loader || <Loader size={"xs"}/>}
        WithError={WithError}
        WithSuccess={({entity}) => <WithSuccess entities={entity}/>}
    />;
};
