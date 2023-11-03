import {withRepositorySchema}        from "@use-pico/repository";
import {ProducerInputMutationSchema} from "./ProducerInputMutationSchema";
import {ProducerInputQuerySchema}    from "./ProducerInputQuerySchema";
import {ProducerInputSchema}         from "./ProducerInputSchema";
import {ProducerInputShapeSchema}    from "./ProducerInputShapeSchema";

export const ProducerInputRepositorySchema = withRepositorySchema({
    entity:   ProducerInputSchema,
    shape:    ProducerInputShapeSchema,
    query:    ProducerInputQuerySchema,
    mutation: ProducerInputMutationSchema,
});
export type ProducerInputRepositorySchema = typeof ProducerInputRepositorySchema;
