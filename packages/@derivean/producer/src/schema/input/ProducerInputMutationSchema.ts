import {type PicoSchema}          from "@use-pico/schema";
import {withMutationSchema}       from "@use-pico/source";
import {ProducerInputQuerySchema} from "./ProducerInputQuerySchema";
import {ProducerInputShapeSchema} from "./ProducerInputShapeSchema";

export const ProducerInputMutationSchema = withMutationSchema({
    shape: ProducerInputShapeSchema,
    query: ProducerInputQuerySchema,
});
export type ProducerInputMutationSchema = typeof ProducerInputMutationSchema;
export namespace ProducerInputMutationSchema {
    export type Type = PicoSchema.Output<ProducerInputMutationSchema>;
}
