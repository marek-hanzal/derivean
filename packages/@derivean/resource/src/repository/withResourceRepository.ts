import {type IContainer}        from "@use-pico/container";
import {withRepositoryHandler}  from "@use-pico/rpc-server";
import {withResourceMutation}   from "../mutation/withResourceMutation";
import {withResourceCountQuery} from "../query/withResourceCountQuery";
import {withResourceQuery}      from "../query/withResourceQuery";

export const withResourceRepository: IContainer.Register = container => {
    withRepositoryHandler({
        container,
        table:    "Resource",
        query:    withResourceQuery,
        count:    withResourceCountQuery,
        mutation: withResourceMutation,
        withFilter({select}) {
            return select;
        }
    });
};
