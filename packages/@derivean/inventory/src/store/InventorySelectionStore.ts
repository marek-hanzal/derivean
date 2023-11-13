import {withDullSchema}       from "@use-pico/dull-stuff";
import {withSelectionStore}   from "@use-pico/selection";
import {type InventorySchema} from "../schema/InventorySchema";

export const InventorySelectionStore = withSelectionStore<withDullSchema.Infer.Entity<InventorySchema>>();
