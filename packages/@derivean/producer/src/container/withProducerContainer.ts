import {type IContainer}              from "@use-pico/container";
import {
    IHandler,
    withRepositoryHandler
}                                     from "@use-pico/rpc-server";
import {withProductionTimeQuery}      from "../query/withProductionTimeQuery";
import {ProducerInputRepository}      from "../repository/ProducerInputRepository";
import {ProducerOutputRepository}     from "../repository/ProducerOutputRepository";
import {ProducerRepository}           from "../repository/ProducerRepository";
import {ProducerInputRpc}             from "../rpc/ProducerInputRpc";
import {ProducerOutputRpc}            from "../rpc/ProducerOutputRpc";
import {ProducerRpc}                  from "../rpc/ProducerRpc";
import {ProducerService}              from "../service/ProducerService";
import {withProducerInputRepository}  from "./withProducerInputRepository";
import {withProducerOutputRepository} from "./withProducerOutputRepository";
import {withProducerRepository}       from "./withProducerRepository";
import {withProducerService}          from "./withProducerService";

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
    withRepositoryHandler({
        container,
        repository:     ProducerOutputRepository,
        withRepository: withProducerOutputRepository,
        handler:        ProducerOutputRpc,
    });

    container.useValue<IHandler<any, any>>(withProductionTimeQuery.key.join("."), {
        schema: withProductionTimeQuery.schema,
        handle: async ({
                           container,
                           request
                       }) => {
            return withProducerService.use(container).timeOf(request.id);
        },
    });
};
