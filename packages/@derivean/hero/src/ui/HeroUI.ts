import {withDullUI}     from "@use-pico/dull-stuff";
import {HeroQueryStore} from "../query/HeroQueryStore";
import {HeroRpc}        from "../rpc/HeroRpc";

export const HeroUI = withDullUI({
    rpc:        HeroRpc,
    queryStore: HeroQueryStore,
});
export type HeroUI = typeof HeroUI;
