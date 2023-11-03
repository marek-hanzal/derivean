import {type IContainer}        from "@use-pico/container";
import {withRepositoryHandler}  from "@use-pico/rpc-server";
import {withProducerMutation}   from "../mutation/withProducerMutation";
import {withProducerCountQuery} from "../query/withProducerCountQuery";
import {withProducerQuery}      from "../query/withProducerQuery";
import {ProducerRepository}     from "../repository/ProducerRepository";
import {ProducerService}        from "../service/ProducerService";
import {withProducerRepository} from "./withProducerRepository";
import {withProducerService}    from "./withProducerService";

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
};
