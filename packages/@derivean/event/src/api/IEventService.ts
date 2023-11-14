export interface IEventService {
    /**
     * Execute given event; if not found, nothing happens.
     */
    execute(kingdomId: string, name: string): Promise<void>;
}
