import {
    type RequestSchema,
    type ResponseSchema
} from "@use-pico/schema";

export interface IRpcHandler<TRequestSchema extends RequestSchema, TResponseSchema extends ResponseSchema> {
    handle(request: TRequestSchema): Promise<TResponseSchema>;
}
