import {
    InventoryRepository,
    withInventoryRepository
}                   from "@derivean/inventory";
import {
    type Database,
    withConnection
}                   from "@derivean/orm";
import {type Infer} from "@use-pico/extras";
import {
    AbstractRepository,
    lazyOf,
    withUserService
}                   from "@use-pico/server";
import {HeroSchema} from "../schema/HeroSchema";

export class HeroRepository extends AbstractRepository<
    Database,
    HeroSchema,
    "Hero"
> {
    static inject = [
        lazyOf(withConnection.inject),
        lazyOf(withUserService.inject),
        lazyOf(withInventoryRepository.inject),
    ];

    constructor(
        connection: withConnection,
        protected userService: withUserService,
        protected inventoryRepository: InventoryRepository.Type
    ) {
        super(
            connection,
            HeroSchema,
            "Hero",
        );
        this.defaultOrderBy = {
            level:    "asc",
            prestige: "asc",
            name:     "asc",
        };
        this.matchOf = {
            userId:    "userId",
            kingdomId: "kingdomId",
        };
    }

    public async toCreate(create: Infer.Create<HeroSchema>): Promise<Infer.EntityWithoutId<HeroSchema>> {
        return {
            ...create,
            inventoryId: create.inventoryId ?? (await this.inventoryRepository.withMutation.create({
                name: `Hero [${create.name}]`,
            })).id,
            userId:      create.userId ?? this.userService.requiredId(),
        };
    }
}

export namespace HeroRepository {
    export type Type = InstanceType<typeof HeroRepository>;
}
