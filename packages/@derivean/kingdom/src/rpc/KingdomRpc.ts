import {withRpc}       from "@use-pico/extras";
import {KingdomSchema} from "../schema/KingdomSchema";

export const KingdomRpc = withRpc({
    key:    ["derivean", "kingdom"],
    schema: KingdomSchema,
});
