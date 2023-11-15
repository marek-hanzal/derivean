import {
    type IInventoryService,
    withInventoryService
}                                                      from "@derivean/inventory";
import {lazyOf}                                        from "@use-pico/container";
import {type IConstructionService}                     from "../api/IConstructionService";
import {withBuildingConstructionRequirementRepository} from "../container/withBuildingConstructionRequirementRepository";
import {withBuildingRequirementRepository}             from "../container/withBuildingRequirementRepository";
import {BuildingConstructionRequirementRepository}     from "../repository/BuildingConstructionRequirementRepository";
import {BuildingRequirementRepository}                 from "../repository/BuildingRequirementRepository";

export class ConstructionService implements IConstructionService {
    static inject = [
        lazyOf(withBuildingConstructionRequirementRepository.inject),
        lazyOf(withBuildingRequirementRepository.inject),
        lazyOf(withInventoryService.inject),
    ];

    constructor(
        protected buildingConstructionRequirementRepository: BuildingConstructionRequirementRepository.Type,
        protected buildingRequirementRepository: BuildingRequirementRepository.Type,
        protected inventoryService: IInventoryService,
    ) {
    }

    public async isAvailable(inventoryId: string, buildingId: string): Promise<IConstructionService.AvailableResult> {
        const items = await this.inventoryService.load(inventoryId);
        const itemIds = items.map(item => item.itemId);
        const constructionItems = await this.buildingConstructionRequirementRepository.withQuery.query({
            where: {
                buildingId,
            },
        });
        const requiredItems = await this.buildingRequirementRepository.withQuery.query({
            where: {
                buildingId,
            },
        });

        const result: IConstructionService.AvailableResult = {
            result:    true,
            missing:   {
                construction: [],
                required:     [],
            },
            notEnough: {
                construction: [],
                required:     [],
            },
        };

        for (const item of constructionItems) {
            if (!itemIds.includes(item.itemId)) {
                result.missing.construction.push(item.itemId);
                result.result = false;
                continue;
            }
            const amount = items.reduce((prev, current) => {
                return item.itemId === current.itemId ? prev + current.amount : prev;
            }, 0);
            if (amount < item.amount) {
                result.notEnough.construction.push({
                    itemId: item.itemId,
                    diff:   item.amount - amount,
                });
                result.result = false;
            }
        }
        for (const item of requiredItems) {
            if (!itemIds.includes(item.itemId)) {
                result.missing.required.push(item.itemId);
                result.result = false;
                continue;
            }
            const amount = items.reduce((prev, current) => {
                return item.itemId === current.itemId ? prev + current.amount : prev;
            }, 0);
            if (amount < item.amount) {
                result.notEnough.required.push({
                    itemId: item.itemId,
                    diff:   item.amount - amount,
                });
                result.result = false;
            }
        }

        return result;
    }
}
