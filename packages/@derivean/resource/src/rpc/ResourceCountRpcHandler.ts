import {CountRpcHandler}          from "@use-pico/repository";
import {type IResourceRepository} from "../api/IResourceRepository";
import {withResourceRepository}   from "../container/withResourceRepository";
import {ResourceSourceSchema}     from "../schema/ResourceSourceSchema";

export class ResourceCountRpcHandler extends CountRpcHandler<IResourceRepository> {
    static inject = [
        withResourceRepository.inject,
    ];

    static $key = "@derivean/resource/ResourceCountRpcHandler";
    static $querySchema = ResourceSourceSchema.shape.query;
    static $mutationSchema = ResourceSourceSchema.shape.mutation;
    static $requestSchema = ResourceSourceSchema.shape.query;

    constructor(
        repository: IResourceRepository,
    ) {
        super(repository);
    }

    public async handle(): Promise<any> {
        return;
    }
}
