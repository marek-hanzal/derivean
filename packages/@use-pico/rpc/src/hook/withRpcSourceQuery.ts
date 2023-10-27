"use client";

import {
    createQueryStore,
    type QuerySchema
}                                        from "@use-pico/query";
import {type IRepository}                from "@use-pico/repository";
import {
    type ArraySchema,
    type ResponseSchema
}                                        from "@use-pico/schema";
import {type IRepositoryRpcHandlerClass} from "../api/IRepositoryRpcHandlerClass";
import {type IRpcHandler}                from "../api/IRpcHandler";
import {type WithSourceQuery}            from "../api/WithSourceQuery";
import {withRpcQuery}                    from "./withRpcQuery";
import {withSourceQuery}                 from "./withSourceQuery";

export const withRpcSourceQuery = <
    TRequestSchema extends QuerySchema<any, any>,
    TResponseSchema extends ArraySchema<ResponseSchema>,
    TRepository extends IRepository<any>,
    THandler extends IRpcHandler<TRequestSchema, TResponseSchema>,
>(
    handler: IRepositoryRpcHandlerClass<TRequestSchema, TResponseSchema, TRepository, THandler>,
): WithSourceQuery<
    TRepository["schema"]["shape"]["entity"],
    TRepository["schema"]["shape"]["filter"],
    TRepository["schema"]["shape"]["orderBy"]
> => {
    return withSourceQuery({
        service:        handler.$key,
        schema:         {
            filter:   handler.$querySchema.shape.filter,
            orderBy:  handler.$querySchema.shape.orderBy,
            response: handler.$responseSchema,
        },
        query:          createQueryStore({
            name:   "SourceQuery",
            schema: {
                filter:  handler.$querySchema.shape.filter,
                orderBy: handler.$querySchema.shape.orderBy,
            },
        }),
        withCountQuery: withRpcQuery(handler.$countRpcHandler) as any,
    });
};
