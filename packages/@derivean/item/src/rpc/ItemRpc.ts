import {withDullRpc} from "@use-pico/dull-stuff";
import {ItemSchema}  from "../schema/ItemSchema";

export const ItemRpc = withDullRpc({
    key:    ["derivean", "item"],
    schema: ItemSchema,
});
export type ItemRpc = typeof ItemRpc;
