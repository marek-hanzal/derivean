import {withDullUI}  from "@use-pico/dull-stuff";
import {ResourceRpc} from "../rpc/ResourceRpc";

export const ResourceUI = withDullUI({
    rpc: ResourceRpc,
});
export type ResourceUI = typeof ResourceUI;
