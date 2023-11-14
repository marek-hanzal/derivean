import {
    type IInventoryService,
    withInventoryService
}                                     from "@derivean/inventory";
import {
    KingdomRepository,
    withKingdomRepository
}                                     from "@derivean/kingdom";
import {lazyOf}                       from "@use-pico/container";
import {type IEventInventoryService}  from "../api/IEventInventoryService";
import {withEventInventoryRepository} from "../container/withEventInventoryRepository";
import {EventInventoryRepository}     from "../repository/EventInventoryRepository";

export class EventInventoryService implements IEventInventoryService {
    static inject = [
        lazyOf(withEventInventoryRepository.inject),
        lazyOf(withKingdomRepository.inject),
        lazyOf(withInventoryService.inject),
    ];

    constructor(
        protected eventInventoryRepository: EventInventoryRepository.Type,
        protected kingdomRepository: KingdomRepository.Type,
        protected inventoryService: IInventoryService,
    ) {
    }

    public async execute(kingdomId: string, eventId: string): Promise<void> {
        const kingdom = await this.kingdomRepository.withQuery.fetchOrThrow({
            where: {
                id: kingdomId,
            },
        });
        const eventInventory = await this.eventInventoryRepository.withQuery.fetchOrThrow({
            where: {
                eventId,
            },
        });
        return this.inventoryService.applyTo(eventInventory.inventoryId, kingdom.inventoryId);
    }
}
