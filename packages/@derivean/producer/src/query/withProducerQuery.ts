import {withRpcSourceQuery}     from "@use-pico/rpc";
import {ProducerSchema}         from "../schema/ProducerSchema";
import {withProducerCountQuery} from "./withProducerCountQuery";

export const withProducerQuery = withRpcSourceQuery({
    key:            ["derivean", "producer", "query"],
    schema:         {
        query:    ProducerSchema.query,
        response: ProducerSchema.entity
    },
    withCountQuery: withProducerCountQuery,
});
