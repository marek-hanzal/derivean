import {withRpc}        from "@use-pico/extras";
import {ItemTypeSchema} from "../schema/ItemTypeSchema";

export const ItemTypeRpc = withRpc({
    key:    ["derivean", "item", "type"],
    schema: ItemTypeSchema,
});
export type ItemTypeRpc = typeof ItemTypeRpc;
