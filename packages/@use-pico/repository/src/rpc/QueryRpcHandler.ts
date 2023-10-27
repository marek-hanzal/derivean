import {AbstractRpcHandler} from "@use-pico/rpc";
import {
    type ArraySchema,
    schema
}                           from "@use-pico/schema";
import {type IRepository}   from "../api/IRepository";

export abstract class QueryRpcHandler<
    TRepository extends IRepository<any>,
> extends AbstractRpcHandler<
    TRepository extends IRepository<infer TSourceSchema> ? TSourceSchema["shape"]["query"] : never,
    TRepository extends IRepository<infer TSourceSchema> ? ArraySchema<TSourceSchema["shape"]["entity"]> : never
> {
    protected constructor(
        protected repository: TRepository,
    ) {
        super(
            repository.schema.shape.mutation,
            schema(z => z.array(repository.schema.shape.entity)) as any,
        );
    }
}
