import {
    type IEventService,
    withEventService
}                      from "@derivean/event";
import {
    type InventoryRepository,
    withInventoryRepository
}                      from "@derivean/inventory";
import {
    type Database,
    withConnection
}                      from "@derivean/orm";
import {Infer}         from "@use-pico/extras";
import {
    AbstractRepository,
    type Connection,
    type IUserService,
    lazyOf,
    withUserService
}                      from "@use-pico/server";
import {utc}           from "@use-pico/utils";
import {KingdomSchema} from "../schema/KingdomSchema";

export class KingdomRepository extends AbstractRepository<
    Database,
    KingdomSchema,
    "Kingdom"
> {
    static inject = [
        lazyOf(withConnection.inject),
        lazyOf(withInventoryRepository.inject),
        lazyOf(withUserService.inject),
        lazyOf(withEventService.inject),
    ];

    constructor(
        connection: Connection<Database>,
        protected inventoryRepository: InventoryRepository.Type,
        protected userService: IUserService,
        protected eventService: IEventService,
    ) {
        super(
            connection,
            KingdomSchema,
            "Kingdom",
        );
        this.defaultOrderBy = {
            name: "asc",
        };
        this.matchOf = {
            name:   "name",
            userId: "userId",
        };
    }

    public async toCreate(create: Infer.Create<KingdomSchema>): Promise<Infer.EntityWithoutId<KingdomSchema>> {
        return {
            ...create,
            created: utc(),
            inventoryId: create.inventoryId || (await this.inventoryRepository.withMutation.create({
                name: `Kingdom [${create.name}]`,
            })).id,
            userId:      this.userService.requiredId(),
        };
    }

    public async onCreate(entity: Infer.Entity<KingdomSchema>): Promise<any> {
        return this.eventService.execute(entity.id, "welcome-gift");
    }
}

export namespace KingdomRepository {
    export type Type = InstanceType<typeof KingdomRepository>;
}
