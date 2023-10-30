import {
    CountSchema,
    withQuery
}                                from "@use-pico2/query";
import {withResourceCountAction} from "../action/withResourceCountAction";
import {ResourceQuerySchema}     from "../schema/ResourceQuerySchema";

export const withResourceCountQuery = withQuery({
    key:      ["derivean", "resource", "count"],
    schema:   {
        request:  ResourceQuerySchema,
        response: CountSchema,
    },
    callback: withResourceCountAction,
});
