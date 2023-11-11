import {withDullUI}         from "@use-pico/dull-stuff";
import {ResourceQueryStore} from "../query/ResourceQueryStore";
import {ResourceRpc}        from "../rpc/ResourceRpc";

export const ResourceUI = withDullUI({
    rpc:        ResourceRpc,
    queryStore: ResourceQueryStore,
});
export type ResourceUI = typeof ResourceUI;
