import {withDullRpc} from "@use-pico/dull-stuff";
import {HeroSchema}  from "../schema/HeroSchema";

export const HeroRpc = withDullRpc({
    key:    ["derivean", "hero"],
    schema: HeroSchema,
});
