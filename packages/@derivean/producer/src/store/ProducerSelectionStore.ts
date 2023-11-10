import {withDullSchema}     from "@use-pico/dull-stuff";
import {withSelectionStore} from "@use-pico/selection";
import {ProducerSchema}     from "../schema/ProducerSchema";

export const ProducerSelectionStore = withSelectionStore<withDullSchema.Infer.Entity<ProducerSchema>>();

