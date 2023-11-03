import {CountSchema}              from "@use-pico/query";
import {withRpcQuery}             from "@use-pico/rpc";
import {ProducerInputQuerySchema} from "../../schema/input/ProducerInputQuerySchema";

export const withProducerInputCountQuery = withRpcQuery({
    key:    ["derivean", "producer", "input", "count"],
    schema: {
        request:  ProducerInputQuerySchema,
        response: CountSchema,
    },
});
