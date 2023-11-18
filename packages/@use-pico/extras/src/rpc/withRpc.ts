import {
    withRpcMutation,
    withRpcQuery,
    withRpcSourceQuery
}                    from "@use-pico/client";
import {CountSchema} from "@use-pico/query";
import {type Schema} from "../api/Schema";

export namespace withRpc {
    export interface Props<
        TSchema extends Schema<any, any, any, any>,
    > {
        /**
         * Base key for all RPC calls.
         */
        key: string[];
        schema: TSchema;
        invalidator?: ReadonlyArray<unknown>;
    }

    export interface Rpc<
        TSchema extends Schema<any, any, any, any>,
    > {
        schema: TSchema;
        query: ReturnType<typeof withRpcSourceQuery<
            TSchema["query"],
            TSchema["entity"]
        >>;
        count: ReturnType<typeof withRpcQuery<
            TSchema["query"],
            CountSchema
        >>;
        mutation: ReturnType<typeof withRpcMutation<
            TSchema["mutation"],
            TSchema["entity"]
        >>;
    }
}

export const withRpc = <
    TSchema extends Schema<any, any, any, any>,
>(
    {
        key,
        schema,
        invalidator,
    }: withRpc.Props<
        TSchema
    >,
) => {
    const count = withRpcQuery({
        key:    key.concat(["count"]),
        schema: {
            request:  schema.query,
            response: CountSchema,
        },
    });
    const query = withRpcSourceQuery({
        key:            key.concat(["query"]),
        schema:         {
            query:    schema.query,
            response: schema.entity
        },
        withCountQuery: count,
    });
    const mutation = withRpcMutation({
        key:         key.concat(["mutation"]),
        schema:      {
            request:  schema.mutation,
            response: schema.entity,
        },
        invalidator: async () => {
            return [
                count.key,
                query.key,
                ...invalidator || [],
            ];
        }
    });

    return {
        schema,
        query,
        count,
        mutation,
    };
};
