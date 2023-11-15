export interface IConstructionService {
    isAvailable(inventoryId: string, buildingId: string): Promise<IConstructionService.AvailableResult>;
}

export namespace IConstructionService {
    export interface AvailableResult {
        result: boolean;
        missing: {
            construction: string[];
            required: string[];
        };
        notEnough: {
            construction: {
                itemId: string;
                diff: number;
            }[];
            required: {
                itemId: string;
                diff: number;
            }[];
        };
    }
}
