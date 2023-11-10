import {CountSchema}         from "@use-pico/query";
import {withRpcQuery}        from "@use-pico/rpc";
import {ProducerInputSchema} from "../../schema/ProducerInputSchema";

export const withProducerInputCountQuery = withRpcQuery({
    key:    ["derivean", "producer", "input", "count"],
    schema: {
        request: ProducerInputSchema.query,
        response: CountSchema,
    },
});
