import {withRepositorySchema}   from "@use-pico/repository";
import {ProducerMutationSchema} from "./ProducerMutationSchema";
import {ProducerQuerySchema}    from "./ProducerQuerySchema";
import {ProducerSchema}         from "./ProducerSchema";
import {ProducerShapeSchema}    from "./ProducerShapeSchema";

export const ProducerRepositorySchema = withRepositorySchema({
    entity:   ProducerSchema,
    shape:    ProducerShapeSchema,
    query:    ProducerQuerySchema,
    mutation: ProducerMutationSchema,
});
export type ProducerRepositorySchema = typeof ProducerRepositorySchema;
