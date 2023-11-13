import {withDullRpc}   from "@use-pico/dull-stuff";
import {KingdomSchema} from "../schema/KingdomSchema";

export const KingdomRpc = withDullRpc({
    key:    ["derivean", "kingdom"],
    schema: KingdomSchema,
});
