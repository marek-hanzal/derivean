import {withQuerySchema}            from "@use-pico/query";
import {type PicoSchema}            from "@use-pico/schema";
import {ProducerInputFilterSchema}  from "./ProducerInputFilterSchema";
import {ProducerInputOrderBySchema} from "./ProducerInputOrderBySchema";

export const ProducerInputQuerySchema = withQuerySchema({
    filter:  ProducerInputFilterSchema,
    orderBy: ProducerInputOrderBySchema,
});
export type ProducerInputQuerySchema = typeof ProducerInputQuerySchema;
export namespace ProducerInputQuerySchema {
    export type Type = PicoSchema.Output<ProducerInputQuerySchema>;
}
