import {type Database}       from "@derivean/orm";
import {lazyOf}              from "@use-pico/container";
import {withDullSchema}      from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                            from "@use-pico/orm";
import {
    AbstractRepository,
    type SelectOf
}                            from "@use-pico/repository";
import {InventoryItemSchema} from "../schema/InventoryItemSchema";

export class InventoryItemRepository extends AbstractRepository<
    Database,
    InventoryItemSchema,
    "InventoryItem"
> {
    static inject = [
        lazyOf(withClient.inject),
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            InventoryItemSchema,
            "InventoryItem",
        );
        this.defaultOrderBy = {};
        this.matchOf = {
            inventoryId: "inventoryId",
            itemId:      "itemId",
        };
    }

    public with<T>(query: withDullSchema.Infer.Query<InventoryItemSchema>, select: SelectOf<Database, "InventoryItem", T>): SelectOf<Database, "InventoryItem", T> {
        return super.with(query, select);
    }
}

export namespace InventoryItemRepository {
    export type Type = InstanceType<typeof InventoryItemRepository>;
}
