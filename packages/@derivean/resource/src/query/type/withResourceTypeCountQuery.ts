import {CountSchema}             from "@use-pico/query";
import {withRpcQuery}            from "@use-pico/rpc";
import {ResourceTypeQuerySchema} from "../../schema/type/ResourceTypeQuerySchema";

export const withResourceTypeCountQuery = withRpcQuery({
    key:    ["derivean", "resource", "type", "count"],
    schema: {
        request:  ResourceTypeQuerySchema,
        response: CountSchema,
    },
});
