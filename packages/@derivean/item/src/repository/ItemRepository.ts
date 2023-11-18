import {
    type Database,
    withConnection
}                   from "@derivean/orm";
import {
    AbstractRepository,
    lazyOf
}                   from "@use-pico/server";
import {ItemSchema} from "../schema/ItemSchema";

export class ItemRepository extends AbstractRepository<
    Database,
    ItemSchema,
    "Item"
> {
    static inject = [
        lazyOf(withConnection.inject),
    ];

    constructor(
        connection: withConnection,
    ) {
        super(
            connection,
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
