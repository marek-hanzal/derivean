import {withSourceQuery}         from "@use-pico2/source";
import {withResourceQueryAction} from "../action/withResourceQueryAction";
import {ResourceQuerySchema}     from "../schema/ResourceQuerySchema";
import {ResourceSchema}          from "../schema/ResourceSchema";
import {withResourceCountQuery}  from "./withResourceCountQuery";

export const withResourceQuery = withSourceQuery({
    key:            ["derivean", "resource", "query"],
    schema:         {
        query:    ResourceQuerySchema,
        response: ResourceSchema,
    },
    withCountQuery: withResourceCountQuery,
    callback: withResourceQueryAction,
});
