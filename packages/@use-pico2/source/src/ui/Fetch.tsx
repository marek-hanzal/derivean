"use client";

import {useParam}        from "@use-pico2/navigation";
import {
    type IWithQuery,
    QueryResult,
    type QuerySchema
}                        from "@use-pico2/query";
import {
    type PicoSchema,
    type WithIdentitySchema
}                        from "@use-pico2/schema";
import {type WithEntity} from "@use-pico2/types";
import {Loader}          from "@use-pico2/ui";
import {
    type FC,
    type ReactNode
}                        from "react";

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
        withQuery: IWithQuery<TQuerySchema, TResponseSchema>;

        /**
         * Error renderer
         */
        WithError?: FC<WithErrorProps>;

        /**
         * Success renderer
         */
        WithSuccess: FC<WithSuccessProps<TResponseSchema>>;
        enabled?: boolean;
        options?: IWithQuery.QueryOptions<TQuerySchema, TResponseSchema>;
    }

    export interface WithErrorProps {
        error: any;
    }

    export interface WithSuccessProps<
        TResponseSchema extends WithIdentitySchema,
    > extends WithEntity.Schema<TResponseSchema> {
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

    const result = withQuery.useQueryEx({
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
        WithLoading={() => loader === undefined ? <Loader type={"dots"} size={"xs"}/> : loader}
        WithError={WithError}
        WithSuccess={({data}) => <WithSuccess entity={data}/>}
    />;
};
