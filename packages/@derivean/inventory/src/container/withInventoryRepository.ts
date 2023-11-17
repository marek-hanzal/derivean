import {withService}              from "@use-pico/server";
import {type InventoryRepository} from "../repository/InventoryRepository";

export const withInventoryRepository = withService<InventoryRepository.Type>("@derivean/inventory/InventoryRepository");
