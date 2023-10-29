import {type IContainer}      from "@use-pico2/container";
import {InventoryService}     from "../service/InventoryService";
import {withInventoryService} from "./withInventoryService";

export const withInventoryContainer: IContainer.Register = container => {
    withInventoryService.bind(container, InventoryService);
};
