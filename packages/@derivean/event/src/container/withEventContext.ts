import {type IContainer}              from "@use-pico/container";
import {withRepositoryHandler}        from "@use-pico/rpc-server";
import {EventInventoryRepository}     from "../repository/EventInventoryRepository";
import {EventRepository}              from "../repository/EventRepository";
import {EventInventoryRpc}            from "../rpc/EventRepositoryRpc";
import {EventRpc}                     from "../rpc/EventRpc";
import {withEventInventoryRepository} from "./withEventInventoryRepository";
import {withEventRepository}          from "./withEventRepository";

export const withEventContext: IContainer.Register = container => {
    withRepositoryHandler({
        container,
        repository:     EventRepository,
        withRepository: withEventRepository,
        handler:        EventRpc,
    });
    withRepositoryHandler({
        container,
        repository:     EventInventoryRepository,
        withRepository: withEventInventoryRepository,
        handler:        EventInventoryRpc,
    });
};
