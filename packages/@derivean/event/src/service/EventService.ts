import {type IEventInventoryService} from "../api/IEventInventoryService";
import {type IEventService}          from "../api/IEventService";
import {withEventInventoryService}   from "../container/withEventInventoryService";
import {withEventRepository}         from "../container/withEventRepository";
import {EventRepository}             from "../repository/EventRepository";

export class EventService implements IEventService {
    static inject = [
        withEventRepository.inject,
        withEventInventoryService.inject,
    ];

    constructor(
        protected eventRepository: EventRepository.Type,
        protected eventInventoryService: IEventInventoryService,
    ) {
    }

    public async execute(kingdomId: string, name: string): Promise<void> {
        try {
            const event = await this.eventRepository.withQuery.fetchOrThrow({
                where: {
                    name,
                },
            });
            switch (event.type) {
                case "EventInventory":
                    return this.eventInventoryService.execute(kingdomId, event.id);
            }
        } catch (e) {
            // swallow, sssht
        }
    }
}
