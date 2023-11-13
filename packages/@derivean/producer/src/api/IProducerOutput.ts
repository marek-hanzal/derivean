import {type IItem} from "@derivean/item";

export interface IProducerOutput {
    /**
     * Item on output
     */
    item: IItem;
    /**
     * Amount of produced item
     */
    amount: number;
}
