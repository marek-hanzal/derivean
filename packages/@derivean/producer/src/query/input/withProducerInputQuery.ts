import {withRpcSourceQuery}          from "@use-pico/rpc";
import {ProducerInputQuerySchema}    from "../../schema/input/ProducerInputQuerySchema";
import {ProducerInputSchema}         from "../../schema/input/ProducerInputSchema";
import {withProducerInputCountQuery} from "./withProducerInputCountQuery";

export const withProducerInputQuery = withRpcSourceQuery({
    key:            ["derivean", "producer", "input", "query"],
    schema:         {
        query:    ProducerInputQuerySchema,
        response: ProducerInputSchema,
    },
    withCountQuery: withProducerInputCountQuery,
});
