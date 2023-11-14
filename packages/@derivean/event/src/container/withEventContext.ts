import {type IContainer}              from "@use-pico/container";
import {withRepositoryHandler}        from "@use-pico/rpc-server";
import {EventInstanceRepository}      from "../repository/EventInstanceRepository";
import {EventInventoryRepository}     from "../repository/EventInventoryRepository";
import {EventRepository}              from "../repository/EventRepository";
import {EventInstanceRpc}             from "../rpc/EventInstanceRpc";
import {EventInventoryRpc}            from "../rpc/EventRepositoryRpc";
import {EventRpc}                     from "../rpc/EventRpc";
import {EventInventoryService}        from "../service/EventInventoryService";
import {EventService}                 from "../service/EventService";
import {withEventInstanceRepository}  from "./withEventInstanceRepository";
import {withEventInventoryRepository} from "./withEventInventoryRepository";
import {withEventInventoryService}    from "./withEventInventoryService";
import {withEventRepository}          from "./withEventRepository";
import {withEventService}             from "./withEventService";

export const withEventContext: IContainer.Register = container => {
    withRepositoryHandler({
        container,
        repository:     EventRepository,
        withRepository: withEventRepository,
        handler:        EventRpc,
    });
    withRepositoryHandler({
        container,
        repository:     EventInstanceRepository,
        withRepository: withEventInstanceRepository,
        handler:        EventInstanceRpc,
    });
    withRepositoryHandler({
        container,
        repository:     EventInventoryRepository,
        withRepository: withEventInventoryRepository,
        handler:        EventInventoryRpc,
    });
    withEventService.bind(container, EventService);
    withEventInventoryService.bind(container, EventInventoryService);
};
