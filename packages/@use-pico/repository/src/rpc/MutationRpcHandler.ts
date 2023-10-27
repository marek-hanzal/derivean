import {AbstractRpcHandler} from "@use-pico/rpc";
import {type IRepository}   from "../api/IRepository";

export abstract class MutationRpcHandler<
    TRepository extends IRepository<any>,
> extends AbstractRpcHandler<
    TRepository extends IRepository<infer TSourceSchema> ? TSourceSchema["shape"]["mutation"] : never,
    TRepository extends IRepository<infer TSourceSchema> ? TSourceSchema["shape"]["entity"] : never
> {
    protected constructor(
        protected repository: TRepository,
    ) {
        super(
            repository.schema.shape.mutation,
            repository.schema.shape.entity
        );
    }
}
