import {type IContainer}      from "@use-pico/container";
import {InventoryService}     from "../service/InventoryService";
import {withInventoryService} from "./withInventoryService";

export const withInventoryContainer: IContainer.Register = container => {
    withInventoryService.bind(container, InventoryService);
};
