import {type IContainer}             from "@use-pico/container";
import {withRepositoryHandler}       from "@use-pico/rpc-server";
import {ProducerInputRepository}     from "../repository/ProducerInputRepository";
import {ProducerRepository}          from "../repository/ProducerRepository";
import {ProducerInputRpc}            from "../rpc/ProducerInputRpc";
import {ProducerRpc}                 from "../rpc/ProducerRpc";
import {ProducerService}             from "../service/ProducerService";
import {withProducerInputRepository} from "./withProducerInputRepository";
import {withProducerRepository}      from "./withProducerRepository";
import {withProducerService}         from "./withProducerService";

export const withProducerContainer = (container: IContainer.Type) => {
    withProducerService.bind(container, ProducerService);

    withRepositoryHandler({
        container,
        repository:     ProducerRepository,
        withRepository: withProducerRepository,
        handler: ProducerRpc,
    });

    withRepositoryHandler({
        container,
        repository:     ProducerInputRepository,
        withRepository: withProducerInputRepository,
        handler: ProducerInputRpc,
    });
};
