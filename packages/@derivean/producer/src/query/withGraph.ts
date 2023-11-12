import {GraphSchema}        from "@use-pico/diagram";
import {withRpcQuery}       from "@use-pico/rpc";
import {WithIdentitySchema} from "@use-pico/schema";

export const withGraph = withRpcQuery({
    key:    ["derivean", "producer", "graph"],
    schema: {
        request:  WithIdentitySchema,
        response: GraphSchema,
    },
});
