import {type Infer}          from "@use-pico/extras";
import {type BuildingSchema} from "../schema/BuildingSchema";

export const BuildingSelectionStore = withSelectionStore<Infer.Entity<BuildingSchema>>();
