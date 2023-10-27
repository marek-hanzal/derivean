import {InventoryResourceSchema} from "../schema/InventoryResourceSchema";
import {type InventorySchema}    from "../schema/InventorySchema";

export interface IInventoryService {
    /**
     * Return normalized inventory with all duplicates summed up
     */
    normalize(inventory: InventorySchema.Type): IInventoryService.Inventory;

    /**
     * Normalize input inventory
     */
    normalizeOf(inventory: InventorySchema.Type): InventorySchema.Type;

    arrayOf(inventory: IInventoryService.Inventory): InventorySchema.Type;

    /**
     * Extract given resource(s) from the inventory
     */
    resourceOf(inventory: InventorySchema.Type, name: string): InventoryResourceSchema.Type[];

    /**
     * Return amount of the given resource in the inventory
     */
    amountOf(inventory: InventorySchema.Type, name: string): number;
}

export namespace IInventoryService {
    /**
     * Normalized inventory
     */
    export type Inventory = Map<string, InventoryResourceSchema.Type>;
}
