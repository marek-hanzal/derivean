import {type IContainer}     from "@use-pico2/container";
import {ProducerService}     from "../service/ProducerService";
import {withProducerService} from "./withProducerService";

export const withProducerContainer = (container: IContainer.Type) => {
    withProducerService.bind(container, ProducerService);
};
