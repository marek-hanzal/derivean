import {
    type IContainer,
    withRepositoryHandler
}                                     from "@use-pico/server";
import {EventHeroRepository}          from "../repository/EventHeroRepository";
import {EventInstanceRepository}      from "../repository/EventInstanceRepository";
import {EventInventoryRepository}     from "../repository/EventInventoryRepository";
import {EventRepository}              from "../repository/EventRepository";
import {EventHeroRpc}                 from "../rpc/EventHeroRpc";
import {EventInstanceRpc}             from "../rpc/EventInstanceRpc";
import {EventInventoryRpc}            from "../rpc/EventRepositoryRpc";
import {EventRpc}                     from "../rpc/EventRpc";
import {EventHeroService}             from "../service/EventHeroService";
import {EventInventoryService}        from "../service/EventInventoryService";
import {EventService}                 from "../service/EventService";
import {withEventHeroRepository}      from "./withEventHeroRepository";
import {withEventHeroService}         from "./withEventHeroService";
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
    withRepositoryHandler({
        container,
        repository:     EventHeroRepository,
        withRepository: withEventHeroRepository,
        handler:        EventHeroRpc,
    });
    withEventService.bind(container, EventService);
    withEventInventoryService.bind(container, EventInventoryService);
    withEventHeroService.bind(container, EventHeroService);
};
