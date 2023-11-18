import {
    IContainer,
    withHandler,
    withRepositoryHandler
}                                     from "@use-pico/server";
import {DependencyError}              from "../error/DependencyError";
import {withDependenciesQuery}        from "../query/withDependenciesQuery";
import {withGraph}                    from "../query/withGraph";
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

export const withProducerContainer: IContainer.Register = container => {
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

    withHandler({
        container,
        key:    withProductionTimeQuery.key,
        schema: withProductionTimeQuery.schema,
        handle: async ({
                           container,
                           request
                       }) => {
            try {
                return await withProducerService.use(container).timeOf(request.id);
            } catch (e) {
                return {
                    time: undefined,
                };
            }
        },
    });
    withHandler({
        container,
        key:    withDependenciesQuery.key,
        schema: withDependenciesQuery.schema,
        handle: async ({
                           container,
                           request
                       }) => {
            try {
                return {
                    producers: await withProducerService.use(container).dependencies(request.id),
                };
            } catch (e) {
                if (e instanceof DependencyError) {
                    return {
                        cycle: e.dependencies,
                    };
                }
                throw e;
            }
        },
    });
    withHandler({
        container,
        key:    withGraph.key,
        schema: withGraph.schema,
        handle: async ({
                           container,
                           request
                       }) => {
            return await withProducerService.use(container).graph(request.id) as any;
        },
    });
};
