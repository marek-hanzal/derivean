import {withDullSchema}     from "@use-pico/dull-stuff";
import {withSelectionStore} from "@use-pico/selection";
import {type KingdomSchema} from "../schema/KingdomSchema";

export const KingdomSelectionStore = withSelectionStore<withDullSchema.Infer.Entity<KingdomSchema>>();
