import {withRpcQuery}       from "@use-pico/client";
import {WithIdentitySchema} from "@use-pico/schema";
import {DependencySchema}   from "../schema/DependencySchema";

export const withDependenciesQuery = withRpcQuery({
    key:    ["derivean", "producer", "dependencies"],
    schema: {
        request:  WithIdentitySchema,
        response: DependencySchema,
    },
});
