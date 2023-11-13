import {type Database}      from "@derivean/orm";
import {withDullSchema}     from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {AbstractRepository} from "@use-pico/repository";
import {ItemTypeSchema}     from "../schema/ItemTypeSchema";

export class ItemTypeRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<ItemTypeSchema>,
    "ItemType"
> {
    static inject = [
        withClient.inject,
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            ItemTypeSchema.repository,
            "ItemType",
        );
        this.defaultOrderBy = {
            name: "asc",
        };
        this.matchOf = {
            name: "name",
        };
    }
}

export namespace ItemTypeRepository {
    export type Type = InstanceType<typeof ItemTypeRepository>;
}
