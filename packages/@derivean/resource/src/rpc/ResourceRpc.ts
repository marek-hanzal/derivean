import {withDullRpc}    from "@use-pico/dull-stuff";
import {ResourceSchema} from "../schema/ResourceSchema";

export const ResourceRpc = withDullRpc({
    key:    ["derivean", "resource"],
    schema: ResourceSchema,
});
export type ResourceRpc = typeof ResourceRpc;
