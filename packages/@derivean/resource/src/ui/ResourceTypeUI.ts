import {withDullUI}      from "@use-pico/dull-stuff";
import {ResourceTypeRpc} from "../rpc/ResourceTypeRpc";

export const ResourceTypeUI = withDullUI({
    rpc: ResourceTypeRpc,
});
export type ResourceTypeUI = typeof ResourceTypeUI;
