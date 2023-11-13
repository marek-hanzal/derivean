import {withService}         from "@use-pico/container";
import {InventoryRepository} from "../repository/InventoryRepository";

export const withInventoryRepository = withService<InventoryRepository.Type>("@derivean/inventory/InventoryRepository");
