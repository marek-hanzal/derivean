import {
    PrismaClient,
    withClient
}                               from "@use-pico2/orm";
import {CountSchema}            from "@use-pico2/query";
import {AbstractRepository}     from "@use-pico2/repository";
import {type PicoSchema}        from "@use-pico2/schema";
import {ResourceMutationSchema} from "../schema/ResourceMutationSchema";
import {ResourceQuerySchema}    from "../schema/ResourceQuerySchema";
import {ResourceSchema}         from "../schema/ResourceSchema";
import {ResourceShapeSchema}    from "../schema/ResourceShapeSchema";

export class ResourceRepository extends AbstractRepository<
    ResourceSchema,
    ResourceShapeSchema,
    ResourceQuerySchema,
    ResourceMutationSchema
> {
    static inject = [
        withClient.inject,
    ];

    constructor(
        client: PrismaClient,
    ) {
        super(
            ResourceSchema,
            ResourceShapeSchema,
            ResourceQuerySchema,
            ResourceMutationSchema,
            client
        );
    }

    public async count(query: ResourceQuerySchema.Type): Promise<CountSchema.Type> {
        return {
            total: await this.prisma.resource.count(),
            where: await this.prisma.resource.count(),
            count: await this.prisma.resource.count(),
        };
    }

    public async query(query: PicoSchema.Output<ResourceQuerySchema>): Promise<PicoSchema.Output<ResourceSchema>[]> {
        return this.prisma.resource.findMany({
            select: {
                id:   true,
                name: true,
            }
        });
    }
}
