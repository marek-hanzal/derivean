import {type Infer}                  from "@use-pico/extras";
import {lazyOf}                      from "@use-pico/server";
import {type IInventory}             from "../api/IInventory";
import {type IInventoryItem}         from "../api/IInventoryItem";
import {type IInventoryService}      from "../api/IInventoryService";
import {withInventoryItemRepository} from "../container/withInventoryItemRepository";
import {type InventoryItemSchema}    from "../schema/InventoryItemSchema";

export class InventoryService implements IInventoryService {
    static inject = [
        lazyOf(withInventoryItemRepository.inject),
    ];

    constructor(
        protected inventoryItemRepository: withInventoryItemRepository,
    ) {
    }

    public normalize(inventory: IInventory): IInventoryService.Inventory {
        const map = new Map<string, IInventoryItem>();
        for (let item of inventory.items) {
            if (!map.has(item.item.name)) {
                map.set(item.item.name, {
                    item:   item.item,
                    amount: 0,
                });
            }
            item["amount"] += (map.get(item.item.name) as typeof item).amount;
        }
        return map;
    }

    public normalizeOf(inventory: IInventory): IInventory {
        return this.arrayOf(
            this.normalize(inventory)
        );
    }

    public arrayOf(inventory: IInventoryService.Inventory): IInventory {
        return {
            items: [...inventory.values()],
        };
    }

    public itemOf(inventory: IInventory, name: string): IInventoryItem[] {
        return inventory.items.filter(({item}) => item.name === name);
    }

    public amountOf(inventory: IInventory, name: string): number {
        return this.itemOf(inventory, name).reduce((amount, item) => {
            return amount + item.amount;
        }, 0);
    }

    public async load(inventoryId: string): Promise<Infer.Entity<InventoryItemSchema>[]> {
        return await this.inventoryItemRepository
            .withQuery
            .select()
            .where("InventoryItem.inventoryId" as any, "=", inventoryId)
            .execute() as Infer.Entity<InventoryItemSchema>[];
    }

    public async applyTo(sourceId: string, targetId: string): Promise<void> {
        for (const {
            id,
            ...item
        } of await this.load(sourceId)) {
            await this.inventoryItemRepository.withMutation.create({
                ...item,
                inventoryId: targetId,
            });
        }
    }
}
