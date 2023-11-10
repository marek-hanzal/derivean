import {dullSchema}           from "@use-pico/dull-stuff";
import {createSelectionStore} from "@use-pico/selection";
import {ResourceTypeSchema}   from "../schema/ResourceTypeSchema";

export const ResourceTypeSelectionStore = createSelectionStore<dullSchema.Infer.Entity<ResourceTypeSchema>>();
