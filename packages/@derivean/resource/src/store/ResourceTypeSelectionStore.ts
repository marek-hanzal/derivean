import {withDullSchema}     from "@use-pico/dull-stuff";
import {withSelectionStore} from "@use-pico/selection";
import {ResourceTypeSchema} from "../schema/ResourceTypeSchema";

export const ResourceTypeSelectionStore = withSelectionStore<withDullSchema.Infer.Entity<ResourceTypeSchema>>();
