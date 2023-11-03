import {type IContainer}             from "@use-pico/container";
import {withRepositoryHandler}       from "@use-pico/rpc-server";
import {withProducerInputMutation}   from "../mutation/withProducerInputMutation";
import {withProducerMutation}        from "../mutation/withProducerMutation";
import {withProducerInputCountQuery} from "../query/input/withProducerInputCountQuery";
import {withProducerInputQuery}      from "../query/input/withProducerInputQuery";
import {withProducerCountQuery}      from "../query/withProducerCountQuery";
import {withProducerQuery}           from "../query/withProducerQuery";
import {ProducerInputRepository}     from "../repository/ProducerInputRepository";
import {ProducerRepository}          from "../repository/ProducerRepository";
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
        handler:        {
            query:    withProducerQuery,
            count:    withProducerCountQuery,
            mutation: withProducerMutation,
        },
    });

    withRepositoryHandler({
        container,
        repository:     ProducerInputRepository,
        withRepository: withProducerInputRepository,
        handler:        {
            query:    withProducerInputQuery,
            count:    withProducerInputCountQuery,
            mutation: withProducerInputMutation,
        },
    });
};
