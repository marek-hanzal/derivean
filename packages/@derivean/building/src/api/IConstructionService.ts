export interface IConstructionService {
    isAvailable(inventoryId: string, buildingId: string): Promise<boolean>;
}
