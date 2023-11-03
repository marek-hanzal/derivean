import {withQuerySchema}       from "@use-pico/query";
import {type PicoSchema}       from "@use-pico/schema";
import {ProducerFilterSchema}  from "./ProducerFilterSchema";
import {ProducerOrderBySchema} from "./ProducerOrderBySchema";

export const ProducerQuerySchema = withQuerySchema({
    filter:  ProducerFilterSchema,
    orderBy: ProducerOrderBySchema,
});
export type ProducerQuerySchema = typeof ProducerQuerySchema;
export namespace ProducerQuerySchema {
    export type Type = PicoSchema.Output<ProducerQuerySchema>;
}
