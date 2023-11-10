import {withRpcSourceQuery}         from "@use-pico/rpc";
import {ResourceTypeSchema}         from "../../schema/ResourceTypeSchema";
import {withResourceTypeCountQuery} from "./withResourceTypeCountQuery";

export const withResourceTypeQuery = withRpcSourceQuery({
    key:            ["derivean", "resource", "type", "query"],
    schema:         {
        query:    ResourceTypeSchema.query,
        response: ResourceTypeSchema.entity,
    },
    withCountQuery: withResourceTypeCountQuery,
});
