import {
    type InventoryRepository,
    withInventoryRepository
}                           from "@derivean/inventory";
import {type Database}      from "@derivean/orm";
import {
    type IUserService,
    withUserService
}                           from "@use-pico/auth-server";
import {withDullSchema}     from "@use-pico/dull-stuff";
import {DateTime}           from "@use-pico/i18n";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {AbstractRepository} from "@use-pico/repository";
import {type PicoSchema}    from "@use-pico/schema";
import {KingdomSchema}      from "../schema/KingdomSchema";

export class KingdomRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<KingdomSchema>,
    "Kingdom"
> {
    static inject = [
        withClient.inject,
        withInventoryRepository.inject,
        withUserService.inject,
    ];

    constructor(
        client: Client<Database>,
        protected inventoryRepository: InventoryRepository.Type,
        protected userService: IUserService,
    ) {
        super(
            client,
            KingdomSchema.repository,
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

    public async toCreate(create: NonNullable<PicoSchema.Output<withDullSchema.Infer.RepositorySchema<KingdomSchema>["mutation"]["shape"]["create"]>>): Promise<Omit<withDullSchema.Infer.Entity<KingdomSchema>, "id">> {
        return {
            ...create,
            created: DateTime.utc().toISO()!,
            inventoryId: create.inventoryId || (await this.inventoryRepository.withMutation.create({
                name: `Kingdom [${create.name}]`,
            })).id,
            userId:      this.userService.requiredId(),
        };
    }
}

export namespace KingdomRepository {
    export type Type = InstanceType<typeof KingdomRepository>;
}
