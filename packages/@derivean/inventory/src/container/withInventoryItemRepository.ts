import {withService}             from "@use-pico/container";
import {InventoryItemRepository} from "../repository/InventoryItemRepository";

export const withInventoryItemRepository = withService<InventoryItemRepository.Type>("@derivean/inventory/InventoryItemRepository");
