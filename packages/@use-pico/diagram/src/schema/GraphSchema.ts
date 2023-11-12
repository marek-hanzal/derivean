import {
    type PicoSchema,
    schema
}                   from "@use-pico/schema";
import {LinkSchema} from "./LinkSchema";
import {NodeSchema} from "./NodeSchema";

export const GraphSchema = schema(z => z.object({
    nodes: z.array(NodeSchema),
    links: z.array(LinkSchema),
}));
export type GraphSchema = typeof GraphSchema;
export namespace GraphSchema {
    export type Type = PicoSchema.Output<GraphSchema>;
}
