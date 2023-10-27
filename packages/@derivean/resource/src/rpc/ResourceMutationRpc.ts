import {MutationRpcHandler}       from "@use-pico/repository";
import {type IResourceRepository} from "../api/IResourceRepository";
import {withResourceRepository}   from "../container/withResourceRepository";

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
}
