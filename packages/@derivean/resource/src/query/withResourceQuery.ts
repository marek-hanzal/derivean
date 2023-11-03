import {withRpcSourceQuery}     from "@use-pico/rpc";
import {ResourceQuerySchema}    from "../schema/ResourceQuerySchema";
import {ResourceSchema}         from "../schema/ResourceSchema";
import {withResourceCountQuery} from "./withResourceCountQuery";

export const withResourceQuery = withRpcSourceQuery({
    key:            ["derivean", "resource", "query"],
    schema:         {
        query:    ResourceQuerySchema,
        response: ResourceSchema,
    },
    withCountQuery: withResourceCountQuery,
});
