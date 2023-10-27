import {MutationRpcHandler}       from "@use-pico/repository";
import {type IResourceRepository} from "../api/IResourceRepository";
import {withResourceRepository}   from "../container/withResourceRepository";
import {ResourceSourceSchema}     from "../schema/ResourceSourceSchema";

export class ResourceMutationRpcHandler extends MutationRpcHandler<IResourceRepository> {
    static inject = [
        withResourceRepository.inject,
    ];

    static $key = "@derivean/resource/ResourceMutationRpcHandler";
    static $requestSchema = ResourceSourceSchema.shape.mutation;
    static $responseSchema = ResourceSourceSchema.shape.entity;

    constructor(
        repository: IResourceRepository,
    ) {
        super(repository);
    }

    public async handle(request: ResourceSourceSchema.Type["mutation"]): Promise<ResourceSourceSchema.Type["entity"]> {
        return {
            name: "dffg",
        };
    }
}
