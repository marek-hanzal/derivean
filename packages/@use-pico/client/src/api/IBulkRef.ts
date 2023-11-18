import {type RpcRequestSchema} from "@use-pico/extras";
import {type ResponseSchema}   from "@use-pico/schema";

export interface IBulkRef {
    schema?: ResponseSchema;
    request: RpcRequestSchema.Type;

    resolve(value: any): void;

    reject(error: any): void;
}
