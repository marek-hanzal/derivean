import {type IContainer}        from "@use-pico/container";
import {
    withCount,
    withQuery
}                               from "@use-pico/repository";
import {withHandler}            from "@use-pico/rpc-server";
import {withResourceCountQuery} from "../query/withResourceCountQuery";
import {withResourceQuery}      from "../query/withResourceQuery";
import {ResourceRepository}     from "../repository/ResourceRepository";

export const withResourceContainer: IContainer.Register = container => {
    withHandler({
        container,
        key:    withResourceQuery.key,
        schema: withResourceQuery.schema,
        handle: async ({
                           container,
                           request
                       }) => {
            return withQuery({
                container,
                request,
                repository: ResourceRepository,
            });
        }
    });

    withHandler({
        container,
        key:    withResourceCountQuery.key,
        schema: withResourceCountQuery.schema,
        handle: async ({
                           container,
                           request
                       }) => {
            return withCount({
                request,
                container,
                repository: ResourceRepository,
            });
        }
    });
};
