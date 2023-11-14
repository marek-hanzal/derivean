import {type Database}      from "@derivean/orm";
import {lazyOf}             from "@use-pico/container";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {AbstractRepository} from "@use-pico/repository";
import {ItemSchema}         from "../schema/ItemSchema";

export class ItemRepository extends AbstractRepository<
    Database,
    ItemSchema,
    "Item"
> {
    static inject = [
        lazyOf(withClient.inject),
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            ItemSchema,
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
