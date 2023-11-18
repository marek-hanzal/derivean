import {
    withRpcMutation,
    withRpcQuery,
    withRpcSourceQuery
}                         from "@use-pico/client";
import {type CountSchema} from "@use-pico/query";
import {type Schema}      from "./Schema";

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
