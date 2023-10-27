"use client";

import {type WithMutation}     from "@use-pico/query";
import {
    type RequestSchema,
    type ResponseSchema
}                              from "@use-pico/schema";
import {useMemo}               from "react";
import {type IRpcHandler}      from "../api/IRpcHandler";
import {type IRpcHandlerClass} from "../api/IRpcHandlerClass";
import {withRpcMutation}       from "./withRpcMutation";

export const useRpcMutation = <
    TRequestSchema extends RequestSchema,
    TResponseSchema extends ResponseSchema,
    THandler extends IRpcHandler<TRequestSchema, TResponseSchema>,
>(
    handler: IRpcHandlerClass<TRequestSchema, TResponseSchema, THandler>,
    props?: WithMutation.Options<
        TRequestSchema,
        TResponseSchema
    >
): WithMutation.Result<
    TRequestSchema,
    TResponseSchema
> => {
    const {useMutation} = useMemo(() => withRpcMutation(handler), [handler.$key]);

    return useMutation(props);
};
