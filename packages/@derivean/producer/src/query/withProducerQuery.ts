import {withRpcSourceQuery}     from "@use-pico/rpc";
import {ProducerQuerySchema}    from "../schema/ProducerQuerySchema";
import {ProducerSchema}         from "../schema/ProducerSchema";
import {withProducerCountQuery} from "./withProducerCountQuery";

export const withProducerQuery = withRpcSourceQuery({
    key:            ["derivean", "producer", "query"],
    schema:         {
        query:    ProducerQuerySchema,
        response: ProducerSchema,
    },
    withCountQuery: withProducerCountQuery,
});
