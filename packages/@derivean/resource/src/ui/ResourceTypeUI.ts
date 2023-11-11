import {withDullUI}             from "@use-pico/dull-stuff";
import {ResourceTypeQueryStore} from "../query/ResourceTypeQueryStore";
import {ResourceTypeRpc}        from "../rpc/ResourceTypeRpc";

export const ResourceTypeUI = withDullUI({
    rpc:        ResourceTypeRpc,
    queryStore: ResourceTypeQueryStore,
});
export type ResourceTypeUI = typeof ResourceTypeUI;
