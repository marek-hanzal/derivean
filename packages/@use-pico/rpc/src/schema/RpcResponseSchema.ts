import {
    ErrorSchema,
    type PicoSchema,
    schema
}                                   from "@use-pico/schema";
import {type withRpcResponseSchema} from "../utils/withRpcResponseSchema";
import {DataResponseSchema}         from "./DataResponseSchema";

export const RpcResponseSchema = schema(z => z.union([
    ErrorSchema,
    DataResponseSchema,
]));
export type RpcResponseSchema<TDataSchema extends PicoSchema> = ReturnType<withRpcResponseSchema<TDataSchema>>;
export namespace RpcResponseSchema {
    export type Type<TDataSchema extends PicoSchema> = PicoSchema.Output<RpcResponseSchema<TDataSchema>>;
}
