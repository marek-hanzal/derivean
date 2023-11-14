export interface IEventInventoryService {
    execute(kingdomId: string, eventId: string): Promise<void>;
}
