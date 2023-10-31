import {type IContainer}        from "@use-pico/container";
import {withRepositoryHandler}  from "@use-pico/rpc-server";
import {withResourceMutation}   from "../mutation/withResourceMutation";
import {withResourceCountQuery} from "../query/withResourceCountQuery";
import {withResourceQuery}      from "../query/withResourceQuery";
import {ResourceRepository}     from "../repository/ResourceRepository";
import {withResourceRepository} from "./withResourceRepository";

export const withResourceContainer: IContainer.Register = container => {
    withRepositoryHandler({
        container,
        repository:     ResourceRepository,
        withRepository: withResourceRepository,
        handler:        {
            query:    withResourceQuery,
            count:    withResourceCountQuery,
            mutation: withResourceMutation,
        },
    });
};
