import {type IInventory}     from "./IInventory";
import {type IInventoryItem} from "./IInventoryItem";

export interface IInventoryService {
    /**
     * Return normalized inventory with all duplicates summed up
     */
    normalize(inventory: IInventory): IInventoryService.Inventory;

    /**
     * Normalize input inventory
     */
    normalizeOf(inventory: IInventory): IInventory;

    arrayOf(inventory: IInventoryService.Inventory): IInventory;

    /**
     * Extract given item(s) from the inventory
     */
    itemOf(inventory: IInventory, name: string): IInventoryItem[];

    /**
     * Return amount of the given item in the inventory
     */
    amountOf(inventory: IInventory, name: string): number;

    /**
     * Copy contents of the source inventory and apply it to target inventory (normalizing it during the process).
     *
     * Source inventory is not modified.
     */
    applyTo(sourceId: string, targetId: string): Promise<void>;
}

export namespace IInventoryService {
    /**
     * Normalized inventory
     */
    export type Inventory = Map<string, IInventoryItem>;
}
