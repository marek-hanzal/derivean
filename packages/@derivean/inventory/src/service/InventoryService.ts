import {type IInventoryService}       from "../api/IInventoryService";
import {type InventoryResourceSchema} from "../schema/InventoryResourceSchema";
import {type InventorySchema}         from "../schema/InventorySchema";

export class InventoryService implements IInventoryService {
    public normalize(inventory: InventorySchema.Type): IInventoryService.Inventory {
        const map = new Map<string, InventoryResourceSchema.Type>();
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

    public normalizeOf(inventory: InventorySchema.Type): InventorySchema.Type {
        return this.arrayOf(
            this.normalize(inventory)
        );
    }

    public arrayOf(inventory: IInventoryService.Inventory): InventorySchema.Type {
        return {
            resources: [...inventory.values()],
        };
    }

    public resourceOf(inventory: InventorySchema.Type, name: string): InventoryResourceSchema.Type[] {
        return inventory.resources.filter(({resource}) => resource.name === name);
    }

    public amountOf(inventory: InventorySchema.Type, name: string): number {
        return this.resourceOf(inventory, name).reduce((amount, resource) => {
            return amount + resource.amount;
        }, 0);
    }
}
