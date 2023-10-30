import {
    CountSchema,
    withQuery
}                            from "@use-pico2/query";
import {ResourceQuerySchema} from "../schema/ResourceQuerySchema";

export const withResourceCountQuery = withQuery({
    key:    ["derivean", "resource", "count"],
    schema: {
        request:  ResourceQuerySchema,
        response: CountSchema,
    },
    async callback(query) {
        return {
            total: 0,
            where: 0,
            count: 0,
        };
    }
});
