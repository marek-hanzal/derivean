import {CountSchema}        from "@use-pico/query";
import {withRpcQuery}       from "@use-pico/rpc";
import {ResourceTypeSchema} from "../../schema/ResourceTypeSchema";

export const withResourceTypeCountQuery = withRpcQuery({
    key:    ["derivean", "resource", "type", "count"],
    schema: {
        request: ResourceTypeSchema.query,
        response: CountSchema,
    },
});
