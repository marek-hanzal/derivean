import {withRpcQuery}         from "@use-pico/client";
import {WithIdentitySchema}   from "@use-pico/schema";
import {ProductionTimeSchema} from "../schema/ProductionTimeSchema";

export const withProductionTimeQuery = withRpcQuery({
    key:    ["derivean", "producer", "production-time"],
    schema: {
        request:  WithIdentitySchema,
        response: ProductionTimeSchema,
    },
});
