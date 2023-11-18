import {withService}             from "@use-pico/server";
import {InventoryItemRepository} from "../repository/InventoryItemRepository";

export const withInventoryItemRepository = withService<InventoryItemRepository.Type>("@derivean/inventory/InventoryItemRepository");
export type withInventoryItemRepository = typeof withInventoryItemRepository["service"];
