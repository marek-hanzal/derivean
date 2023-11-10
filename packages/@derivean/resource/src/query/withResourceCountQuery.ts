import {CountSchema}    from "@use-pico/query";
import {withRpcQuery}   from "@use-pico/rpc";
import {ResourceSchema} from "../schema/ResourceSchema";

export const withResourceCountQuery = withRpcQuery({
    key:    ["derivean", "resource", "count"],
    schema: {
        request: ResourceSchema.query,
        response: CountSchema,
    },
});
