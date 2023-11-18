import {withRpc}    from "@use-pico/extras";
import {ItemSchema} from "../schema/ItemSchema";

export const ItemRpc = withRpc({
    key:    ["derivean", "item"],
    schema: ItemSchema,
});
export type ItemRpc = typeof ItemRpc;
