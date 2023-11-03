import {createSelectionStore} from "@use-pico/selection";
import {type ResourceSchema}  from "../schema/ResourceSchema";

export const ResourceSelectionStore = createSelectionStore<ResourceSchema.Type>();
