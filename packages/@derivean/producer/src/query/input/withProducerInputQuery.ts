import {withRpcSourceQuery}          from "@use-pico/rpc";
import {ProducerInputSchema}         from "../../schema/ProducerInputSchema";
import {withProducerInputCountQuery} from "./withProducerInputCountQuery";

export const withProducerInputQuery = withRpcSourceQuery({
    key:            ["derivean", "producer", "input", "query"],
    schema:         {
        query:    ProducerInputSchema.query,
        response: ProducerInputSchema.entity,
    },
    withCountQuery: withProducerInputCountQuery,
});
