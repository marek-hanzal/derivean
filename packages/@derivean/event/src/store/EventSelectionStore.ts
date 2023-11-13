import {withDullSchema}     from "@use-pico/dull-stuff";
import {withSelectionStore} from "@use-pico/selection";
import {type EventSchema}   from "../schema/EventSchema";

export const EventSelectionStore = withSelectionStore<withDullSchema.Infer.Entity<EventSchema>>();
