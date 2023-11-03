import {type IInventory}         from "./IInventory";
import {type IInventoryResource} from "./IInventoryResource";

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
     * Extract given resource(s) from the inventory
     */
    resourceOf(inventory: IInventory, name: string): IInventoryResource[];

    /**
     * Return amount of the given resource in the inventory
     */
    amountOf(inventory: IInventory, name: string): number;
}

export namespace IInventoryService {
    /**
     * Normalized inventory
     */
    export type Inventory = Map<string, IInventoryResource>;
}
