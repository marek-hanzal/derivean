import {withService}             from "@use-pico/container";
import {ProducerInputRepository} from "../repository/ProducerInputRepository";

export const withProducerInputRepository = withService<ProducerInputRepository.Type>("@derivean/producer/ProducerInputRepository");
