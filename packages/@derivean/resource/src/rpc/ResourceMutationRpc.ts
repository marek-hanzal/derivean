import {MutationRpcHandler}       from "@use-pico/repository";
import {type IResourceRepository} from "../api/IResourceRepository";
import {withResourceRepository}   from "../container/withResourceRepository";
import {ResourceSourceSchema}     from "../schema/ResourceSourceSchema";

export class ResourceMutationRpc extends MutationRpcHandler<IResourceRepository> {
    static inject = [
        withResourceRepository.key,
    ];

    static key = "@derivean/resource/ResourceMutationRpc";

    constructor(
        repository: IResourceRepository,
    ) {
        super(repository);
    }

    public handle(request: ResourceSourceSchema.Type["mutation"]): ResourceSourceSchema.Type["entity"] {
        return {
            name: "dffg",
        };
    }
}
