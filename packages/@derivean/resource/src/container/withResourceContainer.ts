import {type IContainer}        from "@use-pico/container";
import {
    withCountHandler,
    withQueryHandler
}                               from "@use-pico/rpc";
import {withResourceCountQuery} from "../query/withResourceCountQuery";
import {withResourceQuery}      from "../query/withResourceQuery";

export const withResourceContainer: IContainer.Register = container => {
    withQueryHandler({
        container,
        table:   "Resource",
        handler: withResourceQuery,
    });

    withCountHandler({
        container,
        table:   "Resource",
        handler: withResourceCountQuery,
    });
};
