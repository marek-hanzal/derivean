import {QueryRpcHandler}          from "@use-pico/repository";
import {schema}                   from "@use-pico/schema";
import {type IResourceRepository} from "../api/IResourceRepository";
import {withResourceRepository}   from "../container/withResourceRepository";
import {ResourceSourceSchema}     from "../schema/ResourceSourceSchema";

export class ResourceQueryRpcHandler extends QueryRpcHandler<IResourceRepository> {
    static inject = [
        withResourceRepository.inject,
    ];

    static $key = "@derivean/resource/ResourceQueryRpcHandler";
    static $requestSchema = ResourceSourceSchema.shape.query;
    static $responseSchema = schema(z => z.array(ResourceSourceSchema.shape.entity));

    constructor(
        repository: IResourceRepository,
    ) {
        super(repository);
    }

    public async handle<TSourceSchema>(request: any): Promise<any> {
        return [
            {
                name: "bfgfg",
            },
            {
                name: "qqq",
            },
        ];
    }
}
