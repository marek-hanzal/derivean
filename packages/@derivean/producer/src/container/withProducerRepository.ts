import {withService}             from "@use-pico/container";
import {type ProducerRepository} from "../repository/ProducerRepository";

export const withProducerRepository = withService<ProducerRepository.Type>("@derivean/producer/ProducerRepository");
