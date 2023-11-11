import {withDullUI}  from "@use-pico/dull-stuff";
import {BuildingRpc} from "../rpc/BuildingRpc";

export const BuildingUI = withDullUI({
    rpc: BuildingRpc,
});
export type BuildingUI = typeof BuildingUI;
