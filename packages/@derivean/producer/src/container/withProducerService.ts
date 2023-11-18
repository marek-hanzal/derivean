import {withService}           from "@use-pico/server";
import {type IProducerService} from "../api/IProducerService";

export const withProducerService = withService<IProducerService>("@derivean/producer/ProducerService");
export type withProducerService = typeof withProducerService["service"];
