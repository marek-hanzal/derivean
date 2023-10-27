import {AbstractRpcHandler} from "@use-pico/rpc";
import {type IRepository}   from "../api/IRepository";

export abstract class MutationRpcHandler<TRepository extends IRepository<any>> extends AbstractRpcHandler {
    protected constructor(
        protected repository: TRepository,
    ) {
        super();
    }
}
