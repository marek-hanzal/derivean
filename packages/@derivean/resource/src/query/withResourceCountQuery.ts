import {CountSchema}         from "@use-pico/query";
import {withRpcQuery}        from "@use-pico/rpc";
import {ResourceQuerySchema} from "../schema/ResourceQuerySchema";

export const withResourceCountQuery = withRpcQuery({
    key:    ["derivean", "resource", "count"],
    schema: {
        request:  ResourceQuerySchema,
        response: CountSchema,
    },
});
