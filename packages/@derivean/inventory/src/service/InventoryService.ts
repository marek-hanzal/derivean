import {type IInventory}         from "../api/IInventory";
import {type IInventoryResource} from "../api/IInventoryResource";
import {type IInventoryService}  from "../api/IInventoryService";

export class InventoryService implements IInventoryService {
    public normalize(inventory: IInventory): IInventoryService.Inventory {
        const map = new Map<string, IInventoryResource>();
        for (let resource of inventory.resources) {
            if (!map.has(resource.resource.name)) {
                map.set(resource.resource.name, {
                    resource: resource.resource,
                    amount:   0,
                });
            }
            const item = map.get(resource.resource.name) as typeof resource;
            item["amount"] += resource.amount;
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
            resources: [...inventory.values()],
        };
    }

    public resourceOf(inventory: IInventory, name: string): IInventoryResource[] {
        return inventory.resources.filter(({resource}) => resource.name === name);
    }

    public amountOf(inventory: IInventory, name: string): number {
        return this.resourceOf(inventory, name).reduce((amount, resource) => {
            return amount + resource.amount;
        }, 0);
    }
}
