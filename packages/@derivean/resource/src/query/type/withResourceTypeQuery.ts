import {withRpcSourceQuery}         from "@use-pico/rpc";
import {ResourceTypeQuerySchema}    from "../../schema/type/ResourceTypeQuerySchema";
import {ResourceTypeSchema}         from "../../schema/type/ResourceTypeSchema";
import {withResourceTypeCountQuery} from "./withResourceTypeCountQuery";

export const withResourceTypeQuery = withRpcSourceQuery({
    key:            ["derivean", "resource", "type", "query"],
    schema:         {
        query:    ResourceTypeQuerySchema,
        response: ResourceTypeSchema,
    },
    withCountQuery: withResourceTypeCountQuery,
});
