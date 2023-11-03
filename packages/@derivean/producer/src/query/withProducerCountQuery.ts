import {CountSchema}         from "@use-pico/query";
import {withRpcQuery}        from "@use-pico/rpc";
import {ProducerQuerySchema} from "../schema/ProducerQuerySchema";

export const withProducerCountQuery = withRpcQuery({
    key:    ["derivean", "producer", "count"],
    schema: {
        request:  ProducerQuerySchema,
        response: CountSchema,
    },
});
