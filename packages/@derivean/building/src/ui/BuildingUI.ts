import {withDullUI}         from "@use-pico/dull-stuff";
import {BuildingQueryStore} from "../query/BuildingQueryStore";
import {BuildingRpc}        from "../rpc/BuildingRpc";

export const BuildingUI = withDullUI({
    rpc:        BuildingRpc,
    queryStore: BuildingQueryStore,
});
export type BuildingUI = typeof BuildingUI;
