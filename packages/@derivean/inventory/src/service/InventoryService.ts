import {type IInventory}        from "../api/IInventory";
import {type IInventoryItem}    from "../api/IInventoryItem";
import {type IInventoryService} from "../api/IInventoryService";

export class InventoryService implements IInventoryService {
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
}
