import {
    lazyOf,
    withUserService
}                                    from "@use-pico/server";
import {type IEventService}          from "../api/IEventService";
import {withEventHeroService}        from "../container/withEventHeroService";
import {withEventInstanceRepository} from "../container/withEventInstanceRepository";
import {withEventInventoryService}   from "../container/withEventInventoryService";
import {withEventRepository}         from "../container/withEventRepository";

export class EventService implements IEventService {
    static inject = [
        lazyOf(withEventRepository.inject),
        lazyOf(withEventInventoryService.inject),
        lazyOf(withEventInstanceRepository.inject),
        lazyOf(withEventHeroService.inject),
        lazyOf(withUserService.inject),
    ];

    constructor(
        protected eventRepository: withEventRepository,
        protected eventInventoryService: withEventInventoryService,
        protected eventInstanceRepository: withEventInstanceRepository,
        protected eventHeroService: withEventHeroService,
        protected userService: withUserService,
    ) {
    }

    public async execute(kingdomId: string, name: string): Promise<void> {
        const map = {
            EventInventory: this.eventInventoryService,
            EventHero: this.eventHeroService,
        } as const;

        const events = this.eventRepository.withQuery.query({
            where: {name},
        });

        for (const event of await events) {
            try {

                await map[event.type as keyof typeof map]?.execute(kingdomId, event.id);

                await this.eventInstanceRepository.withMutation.create({
                    kingdomId,
                    eventId: event.id,
                    userId:  this.userService.requiredId(),
                    instant: true,
                    commit:  false,
                });
            } catch (e) {
                console.error(e);
            }
        }
    }
}
