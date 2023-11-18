import {withService}            from "@use-pico/server";
import {type IInventoryService} from "../api/IInventoryService";

export const withInventoryService = withService<IInventoryService>("@derivean/inventory/InventoryService");
export type withInventoryService = typeof withInventoryService["service"];
