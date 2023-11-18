import {withService}             from "@use-pico/server";
import {ProducerInputRepository} from "../repository/ProducerInputRepository";

export const withProducerInputRepository = withService<ProducerInputRepository.Type>("@derivean/producer/ProducerInputRepository");
export type withProducerInputRepository = typeof withProducerInputRepository["service"];
