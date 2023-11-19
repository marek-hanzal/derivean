"use client";

import {
    type IWithQuery,
    type QuerySchema
}                        from "@use-pico/query";
import {
    type ArraySchema,
    type PicoSchema,
    type ResponseSchema
}                        from "@use-pico/schema";
import {
    type FC,
    type ReactNode
}                        from "react";
import {type WithEntity} from "../api/WithEntity";
import {useParam}        from "../hook/useParam";
import {useQueryEx}      from "../hook/useQueryEx";
import {Loader}          from "./Loader";
import {QueryResult}     from "./QueryResult";

export namespace Fetch {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TResponseSchema extends ResponseSchema,
    > {
        /**
         * Parameter name from "useParam"; optional
         */
        param?: string;
        /**
         * Override "useParam" and use provided identity
         */
        override?: string;
        query?: PicoSchema.Output<TQuerySchema> | null;
        loader?: ReactNode;
        /**
         * Query to fetch entity
         */
        withQuery: IWithQuery<TQuerySchema, ArraySchema<TResponseSchema>>;

        /**
         * Error renderer
         */
        WithError?: FC<WithErrorProps>;

        /**
         * Success renderer
         */
        WithSuccess?: FC<WithSuccessProps<TResponseSchema>>;
        WithResponse?: FC<WithResponseProps<TResponseSchema>>;
        enabled?: boolean;
        options?: IWithQuery.QueryOptions<TResponseSchema>;
    }

    export interface WithErrorProps {
        error: any;
    }

    export interface WithSuccessProps<
        TResponseSchema extends ResponseSchema,
    > extends WithEntity<NonNullable<PicoSchema.Output<TResponseSchema>>> {
    }

    export interface WithResponseProps<
        TResponseSchema extends ResponseSchema,
    > extends WithEntity.Schema<TResponseSchema> {
    }
}

export const Fetch = <
    TQuerySchema extends QuerySchema<any, any>,
    TResponseSchema extends ResponseSchema,
>(
    {
        param = "id",
        override,
        query,
        loader,
        withQuery,
        WithError = () => null,
        WithSuccess = () => null,
        WithResponse = () => null,
        enabled = true,
        options,
    }: Fetch.Props<TQuerySchema, TResponseSchema>
) => {
    const id = useParam(param, query ? "-" : override);

    const result = useQueryEx({
        withQuery,
        request: query || {
            where: {
                id,
            }
        },
        options: {
            ...options,
            enabled,
        },
    });

    return <QueryResult
        result={result}
        WithLoading={() => loader === undefined ? <Loader size={"xs"}/> : loader}
        WithError={WithError}
        WithSuccess={({entity}) => entity.length > 0 ? <WithSuccess
            entity={entity[0]}
        /> : null}
        WithResponse={({entity}) => <WithResponse
            entity={entity?.[0]}
        />}
    />;
};
