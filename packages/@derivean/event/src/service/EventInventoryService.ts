import {withInventoryService}         from "@derivean/inventory";
import {withKingdomRepository}        from "@derivean/kingdom";
import {lazyOf}                       from "@use-pico/server";
import {type IEventInventoryService}  from "../api/IEventInventoryService";
import {withEventInventoryRepository} from "../container/withEventInventoryRepository";

export class EventInventoryService implements IEventInventoryService {
    static inject = [
        lazyOf(withEventInventoryRepository.inject),
        lazyOf(withKingdomRepository.inject),
        lazyOf(withInventoryService.inject),
    ];

    constructor(
        protected eventInventoryRepository: withEventInventoryRepository,
        protected kingdomRepository: withKingdomRepository,
        protected inventoryService: withInventoryService,
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
