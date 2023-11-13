import {type Database}      from "@derivean/orm";
import {withDullSchema}     from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {AbstractRepository} from "@use-pico/repository";
import {ItemSchema}         from "../schema/ItemSchema";

export class ItemRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<ItemSchema>,
    "Item"
> {
    static inject = [
        withClient.inject,
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            ItemSchema.repository,
            "Item",
        );
        this.defaultOrderBy = {
            name: "asc",
        };
        this.matchOf = {
            name:   "name",
            typeId: "typeId",
        };
    }
}

export namespace ItemRepository {
    export type Type = InstanceType<typeof ItemRepository>;
}
