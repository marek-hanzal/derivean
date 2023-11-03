import {type PicoSchema}     from "@use-pico/schema";
import {withMutationSchema}  from "@use-pico/source";
import {ProducerQuerySchema} from "./ProducerQuerySchema";
import {ProducerShapeSchema} from "./ProducerShapeSchema";

export const ProducerMutationSchema = withMutationSchema({
    shape: ProducerShapeSchema,
    query: ProducerQuerySchema,
});
export type ProducerMutationSchema = typeof ProducerMutationSchema;
export namespace ProducerMutationSchema {
    export type Type = PicoSchema.Output<ProducerMutationSchema>;
}
