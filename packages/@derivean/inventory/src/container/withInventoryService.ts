import {withService}            from "@use-pico/container";
import {type IInventoryService} from "../api/IInventoryService";

export const withInventoryService = withService<IInventoryService>("@derivean/inventory/InventoryService");
