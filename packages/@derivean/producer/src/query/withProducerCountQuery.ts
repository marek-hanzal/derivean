import {CountSchema}    from "@use-pico/query";
import {withRpcQuery}   from "@use-pico/rpc";
import {ProducerSchema} from "../schema/ProducerSchema";

export const withProducerCountQuery = withRpcQuery({
    key:    ["derivean", "producer", "count"],
    schema: {
        request: ProducerSchema.query,
        response: CountSchema,
    },
});
