import {type IInventory}         from "../api/IInventory";
import {type IInventoryItem}     from "../api/IInventoryItem";
import {type IInventoryService}  from "../api/IInventoryService";
import {withInventoryRepository} from "../container/withInventoryRepository";
import {InventoryRepository}     from "../repository/InventoryRepository";

export class InventoryService implements IInventoryService {
    static inject = [
        withInventoryRepository.inject,
    ];

    constructor(
        protected inventoryRepository: InventoryRepository.Type,
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

    public async applyTo(sourceId: string, targetId: string): Promise<void> {


        console.log("Yepyky Yep!");
    }
}
