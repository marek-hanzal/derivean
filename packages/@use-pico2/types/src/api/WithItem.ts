import {type PicoSchema} from "@use-pico2/schema";

export interface WithItem<TItem> {
    item: TItem;
}

export namespace WithItem {
    export type Schema<TSchema extends PicoSchema> = WithItem<PicoSchema.Output<TSchema>>;
}
