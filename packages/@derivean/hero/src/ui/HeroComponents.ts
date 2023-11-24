import {withComponents} from "@use-pico/extras";
import {HeroRpc}        from "../rpc/HeroRpc";
import {HeroQueryStore} from "../store/HeroQueryStore";

export const HeroComponents = withComponents({
    rpc:        HeroRpc,
    queryStore: HeroQueryStore,
});
