import {
    HeroRepository,
    type IHeroService,
    withHeroRepository,
    withHeroService
}                                from "@derivean/hero";
import {
    KingdomRepository,
    withKingdomRepository
}                                from "@derivean/kingdom";
import {lazyOf}                  from "@use-pico/container";
import {type IEventHeroService}  from "../api/IEventHeroService";
import {withEventHeroRepository} from "../container/withEventHeroRepository";
import {EventHeroRepository}     from "../repository/EventHeroRepository";

export class EventHeroService implements IEventHeroService {
    static inject = [
        lazyOf(withEventHeroRepository.inject),
        lazyOf(withKingdomRepository.inject),
        lazyOf(withHeroRepository.inject),
        lazyOf(withHeroService.inject),
    ];

    constructor(
        protected eventHeroRepository: EventHeroRepository.Type,
        protected kingdomRepository: KingdomRepository.Type,
        protected heroRepository: HeroRepository.Type,
        protected heroService: IHeroService,
    ) {
    }

    public async execute(kingdomId: string, eventId: string): Promise<void> {
        const kingdom = await this.kingdomRepository.withQuery.fetchOrThrow({
            where: {
                id: kingdomId,
            },
        });
        const {amount} = await this.eventHeroRepository.withQuery.fetchOrThrow({
            where: {
                eventId,
            },
        });
        for (let i = 0; i < amount; i++) {
            await this.heroRepository.withMutation.create({
                ...(await this.heroService.random()).hero,
                kingdomId: kingdom.id,
            });
        }
    }
}
