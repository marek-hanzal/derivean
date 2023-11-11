import {withDullRpc}        from "@use-pico/dull-stuff";
import {ResourceTypeSchema} from "../schema/ResourceTypeSchema";

export const ResourceTypeRpc = withDullRpc({
    key:    ["derivean", "resource", "type"],
    schema: ResourceTypeSchema,
});
export type ResourceTypeRpc = typeof ResourceTypeRpc;
