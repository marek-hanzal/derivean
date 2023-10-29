import {withService}           from "@use-pico2/container";
import {type IProducerService} from "../api/IProducerService";

export const withProducerService = withService<IProducerService>("@derivean/producer/ProducerService");
