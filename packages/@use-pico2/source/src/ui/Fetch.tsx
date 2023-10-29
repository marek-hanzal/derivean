"use client";

import {useParam}             from "@use-pico2/navigation";
import {
    ErrorResponseSchema,
    type QuerySchema,
    type WithQuery
}                             from "@use-pico2/query";
import {
    type PicoSchema,
    type WithIdentitySchema
}                             from "@use-pico2/schema";
import {type WithEntity}      from "@use-pico2/types";
import {
    type FC,
    type ReactNode
}                             from "react";
import {type IWithFetchQuery} from "../api/IWithFetchQuery";

export namespace Fetch {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TResponseSchema extends WithIdentitySchema,
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
        withQuery: IWithFetchQuery<TQuerySchema, TResponseSchema>;

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

    export interface WithSuccessProps<TResponseSchema extends WithIdentitySchema> extends WithEntity.Schema<TResponseSchema> {
    }
}

export const Fetch = <
    TQuerySchema extends QuerySchema<any, any>,
    TResponseSchema extends WithIdentitySchema,
>(
    {
        param = "id",
        override,
        query,
        loader,
        withQuery,
        WithError = () => null,
        WithSuccess,
        enabled = true,
        options,
    }: Fetch.Props<TQuerySchema, TResponseSchema>
) => {
    const id = useParam(param, query ? "-" : override);

    return "Fetch";

    // const result = withQuery.useQueryEx({
    //     request: query || {
    //         id,
    //     },
    //     options: {
    //         ...options,
    //         enabled,
    //     },
    // });
    // return <QueryResult
    //     result={result}
    //     WithLoading={() => loader === undefined ? <Loader type={"dots"} size={"xs"}/> : loader}
    //     WithError={WithError}
    //     WithSuccess={({data}) => <WithSuccess entity={data}/>}
    // />;
};
