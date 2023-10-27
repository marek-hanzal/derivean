"use client";

import {type WithMutation} from "@use-pico/query";
import {
    type RequestSchema,
    type ResponseSchema
}                          from "@use-pico/schema";
import {useMemo}           from "react";
import {type IRpcHandler}  from "../api/IRpcHandler";
import {withMutation}      from "./withMutation";

export const useRpcMutation = <
    TRequestSchema extends RequestSchema,
    TResponseSchema extends ResponseSchema,
    THandler extends IRpcHandler<TRequestSchema, TResponseSchema>,
>(
    handler: {
                 new(...args: any): THandler,
             } & {
                 $key: string;
                 $requestSchema: TRequestSchema;
                 $responseSchema: TResponseSchema;
             },
    props?: WithMutation.Options<
        TRequestSchema,
        TResponseSchema
    >
): WithMutation.Result<
    TRequestSchema,
    TResponseSchema
> => {
    const {useMutation} = useMemo(() => withMutation({
        service: handler.$key,
        schema:  {
            request:  handler.$requestSchema,
            response: handler.$responseSchema,
        },
    }), [handler.$key]);

    return useMutation(props);
};
