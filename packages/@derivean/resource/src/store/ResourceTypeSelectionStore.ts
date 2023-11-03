import {createSelectionStore}    from "@use-pico/selection";
import {type ResourceTypeSchema} from "../schema/type/ResourceTypeSchema";

export const ResourceTypeSelectionStore = createSelectionStore<ResourceTypeSchema.Type>();
