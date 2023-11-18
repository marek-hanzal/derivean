import {withRpc}    from "@use-pico/extras";
import {HeroSchema} from "../schema/HeroSchema";

export const HeroRpc = withRpc({
    key:    ["derivean", "hero"],
    schema: HeroSchema,
});
