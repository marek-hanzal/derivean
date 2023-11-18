import {withRpcQuery}       from "@use-pico/client";
import {GraphSchema}        from "@use-pico/extras";
import {WithIdentitySchema} from "@use-pico/schema";

export const withGraph = withRpcQuery({
    key:    ["derivean", "producer", "graph"],
    schema: {
        request:  WithIdentitySchema,
        response: GraphSchema,
    },
});
