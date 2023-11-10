import {withDullSchema}      from "@use-pico/dull-stuff";
import {withSelectionStore}  from "@use-pico/selection";
import {type BuildingSchema} from "../schema/BuildingSchema";

export const BuildingSelectionStore = withSelectionStore<withDullSchema.Infer.Entity<BuildingSchema>>();
