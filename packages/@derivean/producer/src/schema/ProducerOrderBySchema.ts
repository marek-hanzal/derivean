import {orderByOf}       from "@use-pico/query";
import {type PicoSchema} from "@use-pico/schema";

export const ProducerOrderBySchema = orderByOf(["name"]);
export type ProducerOrderBySchema = typeof ProducerOrderBySchema;
export namespace ProducerOrderBySchema {
    export type Type = PicoSchema.Output<ProducerOrderBySchema>;
}
