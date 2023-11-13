import {withDullSchema}     from "@use-pico/dull-stuff";
import {withSelectionStore} from "@use-pico/selection";
import {ItemTypeSchema}     from "../schema/ItemTypeSchema";

export const ItemTypeSelectionStore = withSelectionStore<withDullSchema.Infer.Entity<ItemTypeSchema>>();
