import {type IItem} from "@derivean/item";

export interface IProducerInput {
    /**
     * Item on input
     */
    item: IItem;
    /**
     * Amount of processed item (subtracted by time from an inventory)
     */
    amount: number;
}
