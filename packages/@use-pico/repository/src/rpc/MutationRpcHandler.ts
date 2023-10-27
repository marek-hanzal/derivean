import {AbstractRpcHandler} from "@use-pico/rpc";
import {type PicoSchema}    from "@use-pico/schema";
import {type IRepository}   from "../api/IRepository";

export abstract class MutationRpcHandler<
    TRepository extends IRepository<any>,
> extends AbstractRpcHandler<
    TRepository extends IRepository<infer TSourceSchema> ? PicoSchema.Output<TSourceSchema>["mutation"] : never,
    TRepository extends IRepository<infer TSourceSchema> ? PicoSchema.Output<TSourceSchema>["entity"] : never
> {
    protected constructor(
        protected repository: TRepository,
    ) {
        super();
    }
}
