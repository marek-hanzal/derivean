import {withRpcSourceQuery}     from "@use-pico/rpc";
import {ResourceSchema}         from "../schema/ResourceSchema";
import {withResourceCountQuery} from "./withResourceCountQuery";

export const withResourceQuery = withRpcSourceQuery({
    key:            ["derivean", "resource", "query"],
    schema:         {
        query:    ResourceSchema.query,
        response: ResourceSchema.entity
    },
    withCountQuery: withResourceCountQuery,
});
