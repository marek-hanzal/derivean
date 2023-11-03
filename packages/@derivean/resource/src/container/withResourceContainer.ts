import {type IContainer}            from "@use-pico/container";
import {withRepositoryHandler}      from "@use-pico/rpc-server";
import {withResourceMutation}       from "../mutation/withResourceMutation";
import {withResourceTypeMutation}   from "../mutation/withResourceTypeMutation";
import {withResourceTypeCountQuery} from "../query/type/withResourceTypeCountQuery";
import {withResourceTypeQuery}      from "../query/type/withResourceTypeQuery";
import {withResourceCountQuery}     from "../query/withResourceCountQuery";
import {withResourceQuery}          from "../query/withResourceQuery";
import {ResourceRepository}         from "../repository/ResourceRepository";
import {ResourceTypeRepository}     from "../repository/ResourceTypeRepository";
import {withResourceRepository}     from "./withResourceRepository";
import {withResourceTypeRepository} from "./withResourcTypeeRepository";

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
    withRepositoryHandler({
        container,
        repository:     ResourceTypeRepository,
        withRepository: withResourceTypeRepository,
        handler:        {
            query:    withResourceTypeQuery,
            count:    withResourceTypeCountQuery,
            mutation: withResourceTypeMutation,
        },
    });
};
