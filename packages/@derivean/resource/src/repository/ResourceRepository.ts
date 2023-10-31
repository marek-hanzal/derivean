import {
    type Database,
    withClient
}                               from "@derivean/orm";
import {AbstractRepository}     from "@use-pico/repository";
import {ResourceMutationSchema} from "../schema/ResourceMutationSchema";
import {ResourceQuerySchema}    from "../schema/ResourceQuerySchema";
import {ResourceSchema}         from "../schema/ResourceSchema";
import {ResourceShapeSchema}    from "../schema/ResourceShapeSchema";

export class ResourceRepository extends AbstractRepository<
    ResourceSchema,
    ResourceShapeSchema,
    ResourceQuerySchema,
    ResourceMutationSchema,
    Database
> {
    static inject = [
        withClient.inject,
    ];

    constructor(
        client: withClient["service"],
    ) {
        super(
            ResourceSchema,
            ResourceShapeSchema,
            ResourceQuerySchema,
            ResourceMutationSchema,
            client,
            "Resource"
        );
    }
}
