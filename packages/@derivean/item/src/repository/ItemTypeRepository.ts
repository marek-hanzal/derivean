import {
    type Database,
    withConnection
}                       from "@derivean/orm";
import {
    AbstractRepository,
    lazyOf
}                       from "@use-pico/server";
import {ItemTypeSchema} from "../schema/ItemTypeSchema";

export class ItemTypeRepository extends AbstractRepository<
    Database,
    ItemTypeSchema,
    "ItemType"
> {
    static inject = [
        lazyOf(withConnection.inject),
    ];

    constructor(
        connection: withConnection,
    ) {
        super(
            connection,
            ItemTypeSchema,
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
