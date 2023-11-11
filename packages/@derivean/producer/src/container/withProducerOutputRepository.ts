import {withService}              from "@use-pico/container";
import {ProducerOutputRepository} from "../repository/ProducerOutputRepository";

export const withProducerOutputRepository = withService<ProducerOutputRepository.Type>("@derivean/producer/ProducerOutputRepository");
