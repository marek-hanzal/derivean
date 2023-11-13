import {withDullRpc}    from "@use-pico/dull-stuff";
import {ItemTypeSchema} from "../schema/ItemTypeSchema";

export const ItemTypeRpc = withDullRpc({
    key:    ["derivean", "item", "type"],
    schema: ItemTypeSchema,
});
export type ItemTypeRpc = typeof ItemTypeRpc;
