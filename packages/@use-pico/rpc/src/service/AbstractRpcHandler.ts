import {
    type RequestSchema,
    type ResponseSchema
}                         from "@use-pico/schema";
import {type IRpcHandler} from "../api/IRpcHandler";

export abstract class AbstractRpcHandler<TRequestSchema extends RequestSchema, TResponseSchema extends ResponseSchema> implements IRpcHandler<TRequestSchema, TResponseSchema> {
    abstract handle(request: TRequestSchema): Promise<TResponseSchema>;
}
