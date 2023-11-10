import {withDullSchema}      from "@use-pico/dull-stuff";
import {withSelectionStore}  from "@use-pico/selection";
import {type ResourceSchema} from "../schema/ResourceSchema";

export const ResourceSelectionStore = withSelectionStore<withDullSchema.Infer.Entity<ResourceSchema>>();
