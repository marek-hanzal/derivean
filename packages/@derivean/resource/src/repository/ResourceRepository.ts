import {type Database}      from "@derivean/orm";
import {withDullSchema}     from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {AbstractRepository} from "@use-pico/repository";
import {ResourceSchema}     from "../schema/ResourceSchema";

export class ResourceRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<ResourceSchema>,
    "Resource"
> {
    static inject = [
        withClient.inject,
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            ResourceSchema.repository,
            "Resource",
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

export namespace ResourceRepository {
    export type Type = InstanceType<typeof ResourceRepository>;
}
