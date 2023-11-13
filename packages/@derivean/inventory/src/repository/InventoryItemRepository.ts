import {type Database}       from "@derivean/orm";
import {ProducerRepository}  from "@derivean/producer";
import {withDullSchema}      from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                            from "@use-pico/orm";
import {AbstractRepository}  from "@use-pico/repository";
import {InventoryItemSchema} from "../schema/InventoryItemSchema";

export class InventoryItemRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<InventoryItemSchema>,
    "InventoryItem"
> {
    static inject = [
        withClient.inject,
    ];

    constructor(
        client: Client<Database>,
        protected producerRepository: ProducerRepository.Type,
    ) {
        super(
            client,
            InventoryItemSchema.repository,
            "InventoryItem",
        );
        this.defaultOrderBy = {};
        this.matchOf = {
            inventoryId: "inventoryId",
            itemId:      "itemId",
        };
    }
}

export namespace InventoryItemRepository {
    export type Type = InstanceType<typeof InventoryItemRepository>;
}
