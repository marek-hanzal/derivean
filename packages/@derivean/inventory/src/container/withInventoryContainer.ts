import {type IContainer}         from "@use-pico/container";
import {InventoryRepository}     from "../repository/InventoryRepository";
import {InventoryService}        from "../service/InventoryService";
import {withInventoryRepository} from "./withInventoryRepository";
import {withInventoryService}    from "./withInventoryService";

export const withInventoryContainer: IContainer.Register = container => {
    withInventoryService.bind(container, InventoryService);
    withInventoryRepository.bind(container, InventoryRepository);
};
