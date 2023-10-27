import {type WithMutation}        from "@use-pico/query";
import {MutationRpcHandler}       from "@use-pico/repository";
import {withMutation}             from "@use-pico/rpc";
import {type IResourceRepository} from "../api/IResourceRepository";
import {withResourceRepository}   from "../container/withResourceRepository";
import {ResourceSourceSchema}     from "../schema/ResourceSourceSchema";

export class ResourceMutationRpc extends MutationRpcHandler<IResourceRepository> {
    static inject = [
        withResourceRepository.inject,
    ];

    static key = "@derivean/resource/ResourceMutationRpc";

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

    static withMutation: WithMutation<ResourceSourceSchema["shape"]["mutation"], ResourceSourceSchema["shape"]["entity"]> | undefined;

    static useMutation(props?: WithMutation.Options<ResourceSourceSchema["shape"]["mutation"], ResourceSourceSchema["shape"]["entity"]>) {
        return (this.withMutation ?? (this.withMutation = withMutation({
            service: "@derivean/resource/ResourceMutationRpc",
            schema:  {
                request:  ResourceSourceSchema.shape.mutation,
                response: ResourceSourceSchema.shape.entity,
            },
        }))).useMutation(props);
    }
}
