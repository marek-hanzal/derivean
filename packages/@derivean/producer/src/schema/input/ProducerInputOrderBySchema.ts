import {orderByOf}       from "@use-pico/query";
import {type PicoSchema} from "@use-pico/schema";

export const ProducerInputOrderBySchema = orderByOf(["producerId", "amount"]);
export type ProducerInputOrderBySchema = typeof ProducerInputOrderBySchema;
export namespace ProducerInputOrderBySchema {
    export type Type = PicoSchema.Output<ProducerInputOrderBySchema>;
}
