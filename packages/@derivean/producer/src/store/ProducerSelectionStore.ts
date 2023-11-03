import {createSelectionStore} from "@use-pico/selection";
import {type ProducerSchema}  from "../schema/ProducerSchema";

export const ProducerSelectionStore = createSelectionStore<ProducerSchema.Type>();
