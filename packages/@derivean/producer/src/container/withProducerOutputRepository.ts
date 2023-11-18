import {withService}              from "@use-pico/server";
import {ProducerOutputRepository} from "../repository/ProducerOutputRepository";

export const withProducerOutputRepository = withService<ProducerOutputRepository.Type>("@derivean/producer/ProducerOutputRepository");
export type withProducerOutputRepository = typeof withProducerOutputRepository["service"];
