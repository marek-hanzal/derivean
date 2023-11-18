import {withEventService}        from "@derivean/event";
import {withInventoryRepository} from "@derivean/inventory";
import {
    type Database,
    withConnection
}                                from "@derivean/orm";
import {Infer}                   from "@use-pico/extras";
import {
    AbstractRepository,
    lazyOf,
    withUserService
}                                from "@use-pico/server";
import {utc}                     from "@use-pico/utils";
import {KingdomSchema}           from "../schema/KingdomSchema";

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
        connection: withConnection,
        protected inventoryRepository: withInventoryRepository,
        protected userService: withUserService,
        protected eventService: withEventService,
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
