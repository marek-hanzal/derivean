import {
    PrismaClient,
    withClient
}                             from "@use-pico/orm";
import {AbstractRepository}   from "@use-pico/repository";
import {ResourceSourceSchema} from "../schema/ResourceSourceSchema";

export class ResourceRepository extends AbstractRepository<ResourceSourceSchema> {
    static inject = [
        withClient.key,
    ];

    constructor(
        client: PrismaClient,
    ) {
        super(
            ResourceSourceSchema,
            client
        );
    }
}
