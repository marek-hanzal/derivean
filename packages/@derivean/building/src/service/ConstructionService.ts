import {
    type IInventoryService,
    withInventoryService
}                                                      from "@derivean/inventory";
import {lazyOf}                                        from "@use-pico/server";
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

        /**
         * Go through all the items required for a construction
         */
        for (const item of constructionItems) {
            /**
             * See if the required construction item is in an inventory - this is a simple check
             */
            if (!itemIds.includes(item.itemId)) {
                result.missing.construction.push(item.itemId);
                result.result = false;
                continue;
            }
            /**
             * So we have an item in inventory, now we've to check, if we have enough required items.
             *
             * This is a bit more complicated, because we've to go through all the items in inventory and sum them up, because
             * one item type can be in inventory multiple times.
             */
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
        /**
         * Same as for items.
         */
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
