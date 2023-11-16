export interface IEventHeroService {
    execute(kingdomId: string, eventId: string): Promise<void>;
}
