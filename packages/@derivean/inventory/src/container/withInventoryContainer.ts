import {type IContainer}             from "@use-pico/container";
import {withRepositoryHandler}       from "@use-pico/rpc-server";
import {InventoryItemRepository}     from "../repository/InventoryItemRepository";
import {InventoryRepository}         from "../repository/InventoryRepository";
import {InventoryItemRpc}            from "../rpc/InventoryItemRpc";
import {InventoryRpc}                from "../rpc/InventoryRpc";
import {InventoryService}            from "../service/InventoryService";
import {withInventoryItemRepository} from "./withInventoryItemRepository";
import {withInventoryRepository}     from "./withInventoryRepository";
import {withInventoryService}        from "./withInventoryService";

export const withInventoryContainer: IContainer.Register = container => {
    withInventoryService.bind(container, InventoryService);

    withRepositoryHandler({
        container,
        repository:     InventoryRepository,
        withRepository: withInventoryRepository,
        handler:        InventoryRpc,
    });
    withRepositoryHandler({
        container,
        repository:     InventoryItemRepository,
        withRepository: withInventoryItemRepository,
        handler:        InventoryItemRpc,
    });
};
