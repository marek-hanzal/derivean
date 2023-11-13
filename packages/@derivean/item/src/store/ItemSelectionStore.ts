import {withDullSchema}     from "@use-pico/dull-stuff";
import {withSelectionStore} from "@use-pico/selection";
import {type ItemSchema}    from "../schema/ItemSchema";

export const ItemSelectionStore = withSelectionStore<withDullSchema.Infer.Entity<ItemSchema>>();
