import {type Infer}         from "@use-pico/extras";
import {type KingdomSchema} from "../schema/KingdomSchema";

export const KingdomSelectionStore = withSelectionStore<Infer.Entity<KingdomSchema>>();
