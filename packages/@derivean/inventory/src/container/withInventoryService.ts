import {withService}            from "@use-pico2/container";
import {type IInventoryService} from "../api/IInventoryService";

export const withInventoryService = withService<IInventoryService>("@derivean/inventory/InventoryService");
